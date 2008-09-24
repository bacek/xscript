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
#include "xscript/xml_util.h"
#include "xscript/state.h"
#include "xscript/config.h"
#include "xscript/logger.h"
#include "xscript/script.h"
#include "xscript/writer.h"
#include "xscript/context.h"
#include "xscript/authorizer.h"
#include "xscript/profiler.h"
#include "xscript/request_data.h"
#include "xscript/response.h"
#include "xscript/vhost_data.h"
#include "xscript/operation_mode.h"
#include "xscript/xslt_profiler.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

extern "C" int closeFunc(void *ctx);
extern "C" int writeFunc(void *ctx, const char *data, int len);

Server::Server(Config *config) :
        config_(config) {
    config_->startup();
    VirtualHostData::instance()->setServer(this);

    char buf[256];
    int res = ::gethostname(buf, sizeof(buf));
    if (0 == res) {
        hostname_ = std::string(buf);
    }
}

Server::~Server() {
}

bool
Server::isOffline() const {
    return false;
}

void
Server::handleRequest(const boost::shared_ptr<RequestData>& request_data) {

    VirtualHostData::instance()->set(request_data->request());
    XmlUtils::resetReporter();
    xmlOutputBufferPtr buf = NULL;
    const std::string &script_name = request_data->request()->getScriptFilename();
    try {
        PROFILER(log(), "overall time for " + script_name);
        log()->info("requested file: %s", script_name.c_str());

        std::pair<std::string, bool> name = findScript(script_name);

        if (!name.second) {
            OperationMode::instance()->sendError(request_data->response(), 404,
                                                 script_name + " not found");
            return;
        }

        boost::shared_ptr<Script> script = Script::create(name.first);
        if (!script->allowMethod(request_data->request()->getRequestMethod())) {
            OperationMode::instance()->sendError(request_data->response(), 405,
                                                 request_data->request()->getRequestMethod() + " not allowed");
            return;
        }

        Authorizer *authorizer = Authorizer::instance();
        boost::shared_ptr<Context> ctx(createContext(script, request_data));
        ContextStopper ctx_stopper(ctx);
        boost::shared_ptr<AuthContext> auth = authorizer->checkAuth(ctx);
        assert(NULL != auth.get());

        if (!auth->authorized()) {
            authorizer->redirectToAuth(ctx, auth.get());
            ctx->response()->sendHeaders();
            return;
        }
        ctx->authContext(auth);

        XmlDocHelper doc = script->invoke(ctx);
        XmlUtils::throwUnless(NULL != doc.get());

        if (script->binaryPage()) {
            sendHeaders(ctx.get());
            return;
        }

        if (script->forceStylesheet() && needApplyMainStylesheet(ctx->request())) {
            script->applyStylesheet(ctx.get(), doc);
        }

        sendHeaders(ctx.get());

        if (!request_data->request()->suppressBody()) {
            xmlCharEncodingHandlerPtr encoder = NULL;
            const std::string &encoding = ctx->documentWriter()->outputEncoding();
            if (!encoding.empty()) {
                encoder = xmlFindCharEncodingHandler(encoding.c_str());
            }
            buf = xmlOutputBufferCreateIO(&writeFunc, &closeFunc, ctx.get(), encoder);
            XmlUtils::throwUnless(NULL != buf);
            ctx->documentWriter()->write(ctx->response(), doc, buf);
        }

        XsltProfiler::instance()->dumpProfileInfo(ctx.get());
    }
    catch (const std::exception &e) {
        log()->error("%s: exception caught: %s. Owner: %s",
            BOOST_CURRENT_FUNCTION, e.what(), script_name.c_str());
        xmlOutputBufferClose(buf);
        OperationMode::instance()->sendError(request_data->response(), 500, e.what());
    }
}

bool
Server::needApplyMainStylesheet(Request *request) const {
    (void)request;
    return true;
}

bool
Server::needApplyPerblockStylesheet(Request *request) const {
    (void)request;
    return true;
}

Context*
Server::createContext(
    const boost::shared_ptr<Script> &script, const boost::shared_ptr<RequestData> &request_data) {
    return new Context(script, request_data);
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

const std::string&
Server::hostname() const {
    return hostname_;
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

} // namespace xscript
