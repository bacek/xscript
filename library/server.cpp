#include "settings.h"

#include <cerrno>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/current_function.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <unistd.h>
#include <sys/stat.h>

#include <libxml/tree.h>
#include <libxml/xmlsave.h>
#include <libxml/encoding.h>
#include <libxslt/xsltutils.h>

#include "xscript/server.h"
#include "xscript/util.h"
#include "xscript/state.h"
#include "xscript/config.h"
#include "xscript/logger.h"
#include "xscript/script.h"
#include "xscript/writer.h"
#include "xscript/context.h"
#include "xscript/authorizer.h"
#include "xscript/request_data.h"
#include "xscript/response.h"
#include "xscript/vhost_data.h"
#include "xscript/checking_policy.h"
#include "xscript/xslt_profiler.h"
#include "xscript/tag.h"
#include "xscript/doc_cache.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

extern "C" int closeFunc(void *ctx);
extern "C" int writeFunc(void *ctx, const char *data, int len);
extern "C" int writeBufFunc(void *ctx, const char *data, int len);

Server::Server(Config *config) :
        config_(config) {
    config_->startup();
}

Server::~Server() {
}

void
Server::handleRequest(RequestData *request_data) {

    VirtualHostData::instance()->set(request_data->request());
    XmlUtils::resetReporter();
    xmlOutputBufferPtr buf = NULL;
    try {
        log()->info("requested file: %s", request_data->request()->getScriptFilename().c_str());
        std::pair<std::string, bool> name = findScript(request_data->request()->getScriptFilename());

        if (!name.second) {
            CheckingPolicy::instance()->sendError(request_data->response(), 404,
                                                  request_data->request()->getScriptFilename() + " not found");
            return;
        }

        boost::shared_ptr<Script> script = Script::create(name.first);
        if (!script->allowMethod(request_data->request()->getRequestMethod())) {
            CheckingPolicy::instance()->sendError(request_data->response(), 405,
                                                  request_data->request()->getRequestMethod() + " not allowed");
            return;
        }

        Authorizer *authorizer = Authorizer::instance();
        boost::shared_ptr<Context> ctx(new Context(script, *request_data));
        ContextStopper ctx_stopper(ctx);
        boost::shared_ptr<AuthContext> auth = authorizer->checkAuth(ctx);
        assert(NULL != auth.get());

        if (!auth->authorized()) {
            authorizer->redirectToAuth(ctx, auth.get());
            ctx->response()->sendHeaders();
            return;
        }
        ctx->authContext(auth);

        InvokeResult res = script->invoke(ctx);
        XmlUtils::throwUnless(NULL != res.get());

        // TODO We probably have to move all this logick into Script

        XmlDocHelper * doc = res.doc.get();

        if (script->binaryPage()) {
            sendHeaders(ctx.get());
            return;
        }

        if (script->forceStylesheet() && needApplyStylesheet(ctx->request())) {
            // Handle full page cache.
            // We have to store headers and body.
            // So, at the moment, create new trivial doc with full
            // serialized response.
            // TODO It's ugly hack to prove concept. Create big response document
            // and store it in cache
            if (script->tagged()) {
                if (res.cached) {
                    XmlDocHelper loaded_doc;
                    Tag tag;
                    bool l = DocCache::instance()->loadDoc(ctx.get(), script.get(), tag, loaded_doc);
                    assert(l);

                    // Iterate over children of root node. It should be (header*, content);
                    xmlNodePtr root = xmlDocGetRootElement(loaded_doc.get());

                    xmlNodePtr node = root->children;
                    while (node) {
                        if (strcmp((const char*) node->name, "header") == 0) {
                            ctx->response()->setHeader((const char*)node->name, XmlUtils::value(node));
                        }
                        else if (strcmp((const char*) node->name, "content") == 0) {
                            sendHeaders(ctx.get());
                        }
                        else {
                            // Sounds like broken doc
                            assert(!node->name);
                            std::string out = XmlUtils::value(node);
                            ctx->response()->write(out.c_str(), out.length());
                            return;
                        }

                        node = node->next;
                    }

                }
                else {
                    // Apply stylesheet first. It can add more headers.
                    script->applyStylesheet(ctx.get(), *doc);

                    // Store output headers and body for future use
                    XmlDocHelper new_doc(xmlNewDoc((const xmlChar*) "1.0"));
                    xmlNodePtr root = xmlNewDocNode(new_doc.get(), NULL, BAD_CAST "result", 0);
                    XmlUtils::throwUnless(NULL != root);

                    xmlDocSetRootElement(new_doc.get(), root);

                    // Store headers
                    Response::Headers headers = ctx->response()->getHeaders();
                    for(Response::Headers::iterator h = headers.begin(); h != headers.end(); ++h) {
                        XmlNodeHelper header(xmlNewNode(0, BAD_CAST "header"));
                        xmlSetProp(header.get(), BAD_CAST "name", BAD_CAST h->first.c_str());
                        xmlSetProp(header.get(), BAD_CAST "value", BAD_CAST h->second.c_str());
                        xmlAddChild(root, header.get());
                        header.release();
                    }

                    std::string out;
                    
                    xmlCharEncodingHandlerPtr encoder = NULL;
                    const std::string &encoding = ctx->documentWriter()->outputEncoding();
                    if (!encoding.empty()) {
                        encoder = xmlFindCharEncodingHandler(encoding.c_str());
                    }
                    // If there is no errors libxml will close buffer.
                    buf = xmlOutputBufferCreateIO(&writeBufFunc, &closeFunc, &out, encoder);
                    XmlUtils::throwUnless(NULL != buf);
                    ctx->documentWriter()->write(ctx->response(), *doc, buf);

                    // Create single text node for response
                    XmlNodeHelper content(xmlNewNode(0, BAD_CAST "content"));
                    xmlAddChild(content.get(), xmlNewTextLen(BAD_CAST out.c_str(), out.length()));
                    xmlAddChild(root, content.get());
                    content.release();

                    // And finally store doc in cache
                    Tag tag;
                    DocCache::instance()->saveDoc(ctx.get(), script.get(), tag, new_doc);

                    // Now. Send all data to user. He waited enought.
                    sendHeaders(ctx.get());
                    ctx->response()->write(out.c_str(), out.length());
                }

                return;
            }
            else {
                script->applyStylesheet(ctx.get(), *doc);
            }
        }
        else {
            script->removeUnusedNodes(*res.doc.get());
        }

        sendHeaders(ctx.get());

        if (!request_data->request()->suppressBody()) {
            xmlCharEncodingHandlerPtr encoder = NULL;
            const std::string &encoding = ctx->documentWriter()->outputEncoding();
            if (!encoding.empty()) {
                encoder = xmlFindCharEncodingHandler(encoding.c_str());
            }
            // If there is no errors libxml will close buffer.
            buf = xmlOutputBufferCreateIO(&writeFunc, &closeFunc, ctx.get(), encoder);
            XmlUtils::throwUnless(NULL != buf);
            ctx->documentWriter()->write(ctx->response(), *doc, buf);
        }

        XsltProfiler::instance()->dumpProfileInfo(ctx.get());
    }
    catch (const std::exception &e) {
        log()->error("%s: exception caught: %s", BOOST_CURRENT_FUNCTION, e.what());
        xmlOutputBufferClose(buf);
        CheckingPolicy::instance()->sendError(request_data->response(), 500, e.what());
    }
}

bool
Server::needApplyStylesheet(Request *request) const {
    (void)request;
    return true;
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
Server::sendHeaders(Context *ctx) {
    DocumentWriter* writer = ctx->documentWriter();
    assert(NULL != writer);

    writer->addHeaders(ctx->response());
    ctx->response()->sendHeaders();
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
        return context->response()->write(data, len);
    }
    catch (const std::exception &e) {
        log()->error("caught exception while writing result: %s %s",
                     context->request()->getScriptFilename().c_str(), e.what());
    }
    return -1;
}

// Write result of serialisation from libxml to buffer
extern "C" int
writeBufFunc(void *ctx, const char *data, int len) {
    if(len == 0) 
        return 0;
    std::string * str = static_cast<std::string*>(ctx);
    str->append(data, len);
    return len;
}

} // namespace xscript
