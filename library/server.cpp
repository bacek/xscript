#include "settings.h"

#include <cerrno>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <boost/bind.hpp>
#include <boost/current_function.hpp>
#include <boost/function.hpp>
#include <boost/ref.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <unistd.h>
#include <sys/stat.h>

#include <libxml/tree.h>
#include <libxml/xmlsave.h>
#include <libxml/encoding.h>
#include <libxslt/xsltutils.h>

#include "xscript/authorizer.h"
#include "xscript/block.h"
#include "xscript/config.h"
#include "xscript/context.h"
#include "xscript/control_extension.h"
#include "xscript/doc_cache.h"
#include "xscript/logger.h"
#include "xscript/operation_mode.h"
#include "xscript/policy.h"
#include "xscript/request_data.h"
#include "xscript/response.h"
#include "xscript/script.h"
#include "xscript/script_factory.h"
#include "xscript/server.h"
#include "xscript/state.h"
#include "xscript/string_utils.h"
#include "xscript/stylesheet.h"
#include "xscript/stylesheet_factory.h"
#include "xscript/tag.h"
#include "xscript/xml_util.h"
#include "xscript/vhost_data.h"
#include "xscript/writer.h"
#include "xscript/xslt_profiler.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

extern "C" int closeFunc(void *ctx);
extern "C" int writeFunc(void *ctx, const char *data, int len);

class Server::ServerData {
public:
    ServerData(Config *config) : config_(config) {
        config_->startup();

        alternate_port_ = config_->as<unsigned short>("/xscript/alternate-port", 8080);
        noxslt_port_ = config_->as<unsigned short>("/xscript/noxslt-port", 8079);
        
        char buf[256];
        int res = ::gethostname(buf, sizeof(buf));
        if (0 == res) {
            hostname_.assign(buf);
        }
    }
    ~ServerData() {}
    
    Config *config_;
    unsigned short alternate_port_, noxslt_port_;
    std::string hostname_;
};

Server::Server(Config *config) : data_(new ServerData(config)) {
    VirtualHostData::instance()->setServer(this);
}

Server::~Server() {
    delete data_;
}

Config*
Server::config() const {
    return data_->config_;
}

void
Server::handleRequest(const boost::shared_ptr<Request> &request,
                      const boost::shared_ptr<Response> &response,
                      boost::shared_ptr<Context> &ctx) {  
    try {
        VirtualHostData::instance()->set(request.get());
        Context::resetTimer();
        XmlUtils::resetReporter();
        
        std::string url = request->getOriginalUrl();
        log()->info("original url: %s", url.c_str());
        
        boost::shared_ptr<Script> script = getScript(request.get());
        if (NULL == script.get()) {
            OperationMode::sendError(response.get(), 404, request->getScriptFilename() + " not found");
            return;  
        }

        if (!script->allowMethod(request->getRequestMethod())) {
            OperationMode::sendError(response.get(), 405, request->getRequestMethod() + " not allowed");
            return;
        }

        boost::shared_ptr<State> state(new State());
        ctx = boost::shared_ptr<Context>(createContext(script, state, request, response));
        ContextStopper ctx_stopper(ctx);
        Authorizer *authorizer = Authorizer::instance();
        boost::shared_ptr<AuthContext> auth = authorizer->checkAuth(ctx);
        assert(NULL != auth.get());

        ctx->authContext(auth);
        
        if (!auth->authorized()) {
            authorizer->redirectToAuth(ctx, auth.get());
            response->sendHeaders();
            return;
        }

        bool loaded = false;
        bool cachable = script->cachable(ctx.get(), false);
        if (cachable) {
            loaded = processCachedDoc(ctx.get(), script.get());
        }
        
        if (!loaded) {
            XmlDocSharedHelper doc = script->invoke(ctx);
            XmlUtils::throwUnless(NULL != doc->get());
            
            if (script->binaryPage() || response->isBinary()) {
                return;
            }
            
            if (script->forceStylesheet() && !ctx->noMainXsltPort()) {
                script->applyStylesheet(ctx, doc);
                if (response->isBinary()) {
                    return;
                }
            }
            
            if (cachable && script->cachable(ctx.get(), true)) {
                sendResponseCached(ctx.get(), script.get(), doc);
            }
            else {
                sendResponse(ctx.get(), doc);
            }
        }
        
        XsltProfiler::instance()->dumpProfileInfo(ctx);
    }
    catch (const std::exception &e) {
        log()->error("%s: exception caught: %s. Owner: %s",
            BOOST_CURRENT_FUNCTION, e.what(), request->getScriptFilename().c_str());
        OperationMode::sendError(response.get(), 500, e.what());
    }
    catch (...) {
        log()->error("%s: unknown exception caught. Owner: %s",
            BOOST_CURRENT_FUNCTION, request->getScriptFilename().c_str());
        OperationMode::sendError(response.get(), 500, "unknown error");
    }
}

bool
Server::processCachedDoc(Context *ctx, const Script *script) {
    XmlDocSharedHelper doc;
    try {
        Tag tag;
        CacheContext cache_ctx(script);
        if (!PageCache::instance()->loadDoc(ctx, &cache_ctx, tag, doc)) {
            return false;
        }

        xmlNodePtr root = xmlDocGetRootElement(doc->get());
        XmlUtils::throwUnless(NULL != root);
           
        xmlNodePtr cache_data_node = root->last;
        if (NULL == cache_data_node) {
            throw std::runtime_error("Incorrect cache document structure: cache-data node is absent");
        }

        if (strcmp((const char*)(cache_data_node->name), "cache-data") != 0) {
            throw std::runtime_error(
                std::string("Unexpected tag in cached document: ") + (const char*)(cache_data_node->name));
        }
        
        xmlNodePtr node = cache_data_node->children;
        while (node) {
            const char *name = (const char*)(node->name);
            if (strcmp(name, "headers") == 0) {
                xmlNodePtr header_node = node->children;
                while(header_node) {
                    const char *name = (const char*)(header_node->name);
                    if (strcmp(name, "header") == 0) {
                        ctx->response()->setHeader(XmlUtils::attrValue(header_node, "name"),
                                                   XmlUtils::value(header_node));
                    }
                    else {
                        throw std::runtime_error(std::string("Unexpected tag in cached document: ") + name);
                    }
                    header_node = header_node->next;
                }
            }
            else if (strcmp(name, "context-data") == 0) {
                xmlNodePtr context_node = node->children;
                while(context_node) {
                    const char *name = (const char*)(context_node->name);
                    if (strcmp(name, "expire-time-delta") == 0) {
                        ctx->expireTimeDelta(
                            boost::lexical_cast<unsigned int>(XmlUtils::value(context_node)));
                    }
                    else {
                        throw std::runtime_error(std::string("Unexpected tag in cached document: ") + name);
                    }
                    context_node = context_node->next;
                }
            }
            else {
                throw std::runtime_error(std::string("Unexpected tag in cached document: ") + name);
            }
            node = node->next;
        }
               
        xmlUnlinkNode(cache_data_node);
        xmlFreeNode(cache_data_node);
    }
    catch(const std::exception &e) {
        log()->error("Error in loading cached page: %s", e.what());
        return false;
    }
    
    std::string xslt = ctx->xsltName();
    if (!xslt.empty()) {
        boost::shared_ptr<Stylesheet> stylesheet(StylesheetFactory::createStylesheet(xslt));   
        ctx->createDocumentWriter(stylesheet);
    }

    script->addExpiresHeader(ctx);
    sendResponse(ctx, doc);
    
    return true;
}

void
Server::sendResponse(Context *ctx, XmlDocSharedHelper doc) {
    sendHeaders(ctx);
    if (ctx->suppressBody()) {
        return;
    } 

    xmlCharEncodingHandlerPtr encoder = NULL;
    const std::string &encoding = ctx->documentWriter()->outputEncoding();
    if (!encoding.empty()) {
        encoder = xmlFindCharEncodingHandler(encoding.c_str());
    }

    xmlOutputBufferPtr buf = xmlOutputBufferCreateIO(writeFunc, closeFunc, ctx, encoder);
    XmlUtils::throwUnless(NULL != buf);
    
    try {
        ctx->documentWriter()->write(ctx->response(), doc->get(), buf);
    }
    catch(const std::exception &e) {
        xmlOutputBufferClose(buf);
        throw e;
    }
}

void
Server::sendResponseCached(Context *ctx, const Script *script, XmlDocSharedHelper doc) {    
    addHeaders(ctx);
    if (script->cacheTime() < PageCache::instance()->minimalCacheTime()) {
        sendResponse(ctx, doc);
        return;
    }
       
    XmlNodeHelper headers_node(xmlNewNode(NULL, (const xmlChar*)"headers"));
    XmlNodeHelper context_data_node(xmlNewNode(NULL, (const xmlChar*)"context-data"));
    try {
        const HeaderMap& headers = ctx->response()->outHeaders();
        for(HeaderMap::const_iterator h = headers.begin(); h != headers.end(); ++h) {
            if (strncasecmp(h->first.c_str(), "expires", sizeof("expires")) == 0) {
                continue;
            }
            xmlNodePtr header = xmlNewChild(
                headers_node.get(), NULL, (const xmlChar*)"header", (const xmlChar*)h->second.c_str());
            xmlSetProp(header, (const xmlChar*)"name", (const xmlChar*)h->first.c_str());
        }
        
        xmlNewChild(context_data_node.get(), NULL,
                    (const xmlChar*)"expire-time-delta",
                    (const xmlChar*)boost::lexical_cast<std::string>(ctx->expireTimeDelta()).c_str());
    }
    catch(const std::exception &e) {
        log()->error("Error in collecting headers for cache: %s", e.what());
        sendResponse(ctx, doc);
        return;
    }
        
    XmlNodeHelper cache_data_node(xmlNewNode(NULL, (const xmlChar*)"cache-data"));
    xmlAddChild(cache_data_node.get(), headers_node.get());
    headers_node.release();
    xmlAddChild(cache_data_node.get(), context_data_node.get());
    context_data_node.release();
    
    xmlNodePtr root = xmlDocGetRootElement(doc->get());
    xmlAddChild(root, cache_data_node.get());
    xmlNodePtr cache_node = cache_data_node.release();

    try {
        Tag tag;
        tag.expire_time = time(NULL) + script->cacheTime();
        CacheContext cache_ctx(script);
        PageCache::instance()->saveDoc(ctx, &cache_ctx, tag, doc);
    }
    catch(const std::exception &e) {
        log()->error("Error in saving page to cache: %s", e.what());
    }
    
    xmlUnlinkNode(cache_node);
    xmlFreeNode(cache_node);
    
    sendResponse(ctx, doc);
}

boost::shared_ptr<Script>
Server::getScript(Request *request) {
    std::pair<std::string, bool> name = findScript(request->getScriptFilename());
    if (!name.second) {
        return boost::shared_ptr<Script>();
    }
    return ScriptFactory::createScript(name.first);
}

bool
Server::needApplyMainStylesheet(Request *request) const {
    unsigned short port = request->getServerPort();
    return (port != data_->alternate_port_) && (port != data_->noxslt_port_);
}

bool
Server::needApplyPerblockStylesheet(Request *request) const {
    return (request->getServerPort() != data_->noxslt_port_);
}

Context*
Server::createContext(const boost::shared_ptr<Script> &script,
                      const boost::shared_ptr<State> &state,
                      const boost::shared_ptr<Request> &request,
                      const boost::shared_ptr<Response> &response) {
    return new Context(script, state, request, response);
}

std::pair<std::string, bool>
Server::findScript(const std::string &name) {

    namespace fs = boost::filesystem;
    fs::path path(name);
    bool path_exists = fs::exists(path);

    if (!path_exists || !fs::is_directory(path)) {
        return std::make_pair(path.native_file_string(), path_exists);
    }

    fs::path path_local = path / "index.html";
    path_exists = fs::exists(path_local);
    if (path_exists) {
        return std::make_pair(path_local.native_file_string(), path_exists);
    }

    path_local = path / "index.xml";
    path_exists = fs::exists(path_local);
    return std::make_pair(path_local.native_file_string(), path_exists);
}

void
Server::addHeaders(Context *ctx) {
    DocumentWriter* writer = ctx->documentWriter();
    assert(NULL != writer);
    writer->addHeaders(ctx->response());
}

void
Server::sendHeaders(Context *ctx) {
    addHeaders(ctx);
    ctx->response()->sendHeaders();
}

const std::string&
Server::hostname() const {
    return data_->hostname_;
}

unsigned short
Server::alternatePort() const {
    return data_->alternate_port_;
}

unsigned short
Server::noXsltPort() const {
    return data_->noxslt_port_;
}

extern "C" int
closeFunc(void *ctx) {
    (void)ctx;
    return 0;
}

extern "C" int
writeFunc(void *ctx, const char *data, int len) {
    if (0 == len) {
        return 0;
    }
    Context *context = static_cast<Context*>(ctx);
    try {
        return context->response()->write(data, len, context->request());
    }
    catch (const std::exception &e) {
        log()->error("caught exception while writing result: %s %s",
                     context->request()->getScriptFilename().c_str(), e.what());
    }
    return -1;
}

} // namespace xscript
