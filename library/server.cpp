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

#include "xscript/authorizer.h"
#include "xscript/config.h"
#include "xscript/context.h"
#include "xscript/logger.h"
#include "xscript/operation_mode.h"
#include "xscript/profiler.h"
#include "xscript/request_data.h"
#include "xscript/response.h"
#include "xscript/script.h"
#include "xscript/server.h"
#include "xscript/state.h"
#include "xscript/string_utils.h"
#include "xscript/xml_util.h"
#include "xscript/vhost_data.h"
#include "xscript/writer.h"
#include "xscript/xslt_profiler.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

const std::string Server::MAX_BODY_LENGTH = "MAX_BODY_LENGTH";

extern "C" int closeFunc(void *ctx);
extern "C" int writeFunc(void *ctx, const char *data, int len);

Server::Server(Config *config) :
        config_(config) {
    config_->startup();
    
    max_body_length_ = config_->as<std::streamsize>("/xscript/max-body-length",
            std::numeric_limits<std::streamsize>::max());

    alternate_port_ = config_->as<unsigned short>("/xscript/alternate-port", 8080);
    noxslt_port_ = config_->as<unsigned short>("/xscript/noxslt-port", 8079);
    
    char buf[256];
    int res = ::gethostname(buf, sizeof(buf));
    if (0 == res) {
        hostname_.assign(buf);
    }
    
    VirtualHostData::instance()->setServer(this);
}

Server::~Server() {
}

void
Server::handleRequest(const boost::shared_ptr<RequestData> &request_data) {
    VirtualHostData::instance()->set(request_data->request());
    Context::resetTimer();
    XmlUtils::resetReporter();
    xmlOutputBufferPtr buf = NULL;
    const std::string &script_name = request_data->request()->getScriptFilename();
    try {
        std::string url = request_data->request()->getOriginalUrl();
        log()->info("original url: %s", url.c_str());
        
        PROFILER(log(), "overall time for " + script_name);
        log()->info("requested file: %s", script_name.c_str());

        boost::shared_ptr<Script> script = getScript(script_name, request_data->request());
        if (NULL == script.get()) {
            OperationMode::instance()->sendError(request_data->response(), 404,
                                                 script_name + " not found");
            return;  
        }

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
        
        if (script->binaryPage() || request_data->response()->isBinary()) {
            return;
        }
        
        if (script->forceStylesheet() && needApplyMainStylesheet(ctx->request())) {
            script->applyStylesheet(ctx.get(), doc);
            if (request_data->response()->isBinary()) {
                return;
            }
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

boost::shared_ptr<Script>
Server::getScript(const std::string &script_name, Request *request) {
    (void)request;
    
    std::pair<std::string, bool> name = findScript(script_name);
    if (!name.second) {
        return boost::shared_ptr<Script>();
    }

    return Script::create(name.first);
}

bool
Server::needApplyMainStylesheet(Request *request) const {
    unsigned short port = request->getServerPort();
    return (port != alternate_port_) && (port != noxslt_port_);
}

bool
Server::needApplyPerblockStylesheet(Request *request) const {
    return (request->getServerPort() != noxslt_port_);
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

std::streamsize
Server::maxBodyLength(Request *request) const {
    std::string length = VirtualHostData::instance()->getVariable(request, MAX_BODY_LENGTH);
    if (!length.empty()) {
        try {
            return boost::lexical_cast<std::streamsize>(length);
        }
        catch(const boost::bad_lexical_cast &e) {
            log()->error("Cannot parse MAX_BODY_LENGTH environment variable: %s", length.c_str());
        }
    }
    return max_body_length_;
}

unsigned short
Server::alternatePort() const {
    return alternate_port_;
}

unsigned short
Server::noXsltPort() const {
    return noxslt_port_;
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
