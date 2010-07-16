#include "settings.h"

#include <algorithm>
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
#include <boost/tokenizer.hpp>

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
#include "xscript/http_utils.h"
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

#include "xscript/profiler.h"

namespace xscript {

extern "C" int closeFunc(void *ctx);
extern "C" int writeFunc(void *ctx, const char *data, int len);

class Server::ServerData {
public:
    ServerData(Config *config) : config_(config) {
        config_->startup();
        getPorts("/xscript/alternate-port", 8080, alternate_ports_);
        getPorts("/xscript/noxslt-port", 8079, noxslt_ports_);
        
        char buf[256];
        int res = ::gethostname(buf, sizeof(buf));
        if (0 == res) {
            hostname_.assign(buf);
        }
    }
    ~ServerData() {}
    void getPorts(const std::string &node_name, unsigned short def,
            std::vector<unsigned short> &result);
    
    Config *config_;
    std::vector<unsigned short> alternate_ports_;
    std::vector<unsigned short> noxslt_ports_;
    std::string hostname_;
};

void
Server::ServerData::getPorts(const std::string &node_name, unsigned short def,
    std::vector<unsigned short> &result) {

    std::string ports = config_->as<std::string>(
            node_name, boost::lexical_cast<std::string>(def));

    typedef boost::char_separator<char> Separator;
    typedef boost::tokenizer<Separator> Tokenizer;
    Tokenizer tok(ports, Separator(", "));
    try {
        for (Tokenizer::iterator it = tok.begin(), it_end = tok.end();
            it != it_end;
            ++it) {
            result.push_back(boost::lexical_cast<unsigned short>(*it));
        }
    }
    catch(const std::exception &e) {
        result.clear();
        result.push_back(def);
    }
}

Server::Server(Config *config) : data_(new ServerData(config)) {
    VirtualHostData::instance()->setServer(this);
}

Server::~Server() {
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
            OperationMode::instance()->sendError(response.get(), 404, request->getScriptFilename() + " not found");
            return;  
        }

        if (!script->allowMethod(request->getRequestMethod())) {
            OperationMode::instance()->sendError(response.get(), 405, request->getRequestMethod() + " not allowed");
            return;
        }

        boost::shared_ptr<State> state(new State());
        ctx = boost::shared_ptr<Context>(createContext(script, state, request, response));
        ContextStopper ctx_stopper(ctx);
        Authorizer *authorizer = Authorizer::instance();
        boost::shared_ptr<AuthContext> auth = authorizer->checkAuth(ctx.get());
        assert(NULL != auth.get());

        ctx->authContext(auth);
        
        if (!auth->authorized()) {
            authorizer->redirectToAuth(ctx.get(), auth.get());
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
            XmlUtils::throwUnless(NULL != doc.get());
            
            if (script->binaryPage() || response->isBinary()) {
                return;
            }
            
            bool result = true;
            if (script->forceStylesheet() && !ctx->noMainXsltPort() && ctx->hasXslt()) {
                result = script->applyStylesheet(ctx, doc);
                if (response->isBinary()) {
                    ctx->addDoc(doc);
                    return;
                }
            }
            
            ctx->addDoc(doc);

            if (result && cachable && script->cachable(ctx.get(), true)) {
                ctx->response()->setCacheable();
            }

            sendResponse(ctx.get(), doc);
        }
        
        XsltProfiler::instance()->dumpProfileInfo(ctx);
    }
    catch (const std::exception &e) {
        log()->error("%s: exception caught: %s. Owner: %s",
            BOOST_CURRENT_FUNCTION, e.what(), request->getScriptFilename().c_str());
        OperationMode::instance()->sendError(response.get(), 500, e.what());
    }
    catch (...) {
        log()->error("%s: unknown exception caught. Owner: %s",
            BOOST_CURRENT_FUNCTION, request->getScriptFilename().c_str());
        OperationMode::instance()->sendError(response.get(), 500, "unknown error");
    }
}

bool
Server::processCachedDoc(Context *ctx, Script *script) {
    try {       
        Tag tag(true, 1, 1); // fake undefined Tag
        CacheContext cache_ctx(script, ctx, script->allowDistributed());
        boost::shared_ptr<PageCacheData> cache_data =
            PageCache::instance()->loadDoc(&cache_ctx, tag);
        
        if (NULL == cache_data.get()) {
            return false;
        }

        const std::string& if_none_match =
            ctx->request()->getHeader(HttpUtils::IF_NONE_MATCH_HEADER_NAME);

        if (if_none_match != cache_data->etag()) {
            return false;
        }

        ctx->response()->setCacheable(cache_data);
    }
    catch(const std::exception &e) {
        log()->error("Error in loading cached page: %s", e.what());
        return false;
    }

    return true;
}

void
Server::sendResponse(Context *ctx, XmlDocSharedHelper doc) {
    sendHeaders(ctx);
    if (ctx->response()->suppressBody(ctx->request())) {
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
        ctx->documentWriter()->write(ctx->response(), doc.get(), buf);
    }
    catch(const std::exception &e) {
        xmlOutputBufferClose(buf);
        throw e;
    }
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
Server::isAlternatePort(unsigned short port) const {
    return data_->alternate_ports_.end() !=
        std::find(data_->alternate_ports_.begin(), data_->alternate_ports_.end(), port);
}

bool
Server::isNoXsltPort(unsigned short port) const {
    return data_->noxslt_ports_.end() !=
        std::find(data_->noxslt_ports_.begin(), data_->noxslt_ports_.end(), port);
}

bool
Server::needApplyMainStylesheet(Request *request) const {
    unsigned short port = request->getServerPort();
    return !isAlternatePort(port) && !isNoXsltPort(port);
}

bool
Server::needApplyPerblockStylesheet(Request *request) const {
    return !isNoXsltPort(request->getServerPort());
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
