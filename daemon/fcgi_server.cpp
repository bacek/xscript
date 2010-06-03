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

#include <fcgio.h>
#include <fcgiapp.h>

#include <libxml/tree.h>
#include <libxml/xmlsave.h>
#include <libxml/encoding.h>
#include <libxslt/xsltutils.h>

#include "fcgi_server.h"

#include "xscript/authorizer.h"
#include "xscript/config.h"
#include "xscript/context.h"
#include "xscript/logger.h"
#include "xscript/operation_mode.h"
#include "xscript/profiler.h"
#include "xscript/request_data.h"
#include "xscript/script.h"
#include "xscript/state.h"
#include "xscript/status_info.h"
#include "xscript/vhost_data.h"
#include "xscript/writer.h"
#include "xscript/xml_util.h"

#include "internal/response_time_counter_block.h"
#include "internal/response_time_counter_impl.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class FCGIServer::RequestAcceptor {
    public:
        explicit RequestAcceptor(FCGX_Request *req);
        ~RequestAcceptor();

        bool accepted() const { return NULL != req_; }

    private:
        FCGX_Request *req_;
};



FCGIServer::FCGIServer(Config *config) :
        Server(config), socket_(-1), inbuf_size_(0), outbuf_size_(0),
        workerCounter_(SimpleCounterFactory::instance()->createCounter("fcgi-workers", true)),
        uptimeCounter_(),
        responseCounter_(boost::shared_ptr<ResponseTimeCounter>(
                ResponseTimeCounterFactory::instance()->createCounter("response-time").release()))
{
    if (0 != FCGX_Init()) {
        throw std::runtime_error("can not init fastcgi library");
    }
    StatusInfo::instance()->getStatBuilder().addCounter(workerCounter_.get());
    StatusInfo::instance()->getStatBuilder().addCounter(&uptimeCounter_);
    
    responseCounter_ = boost::shared_ptr<ResponseTimeCounter>(
        ResponseTimeCounterFactory::instance()->createCounter("response-time").release());
    
    ControlExtension::Constructor f =
        boost::bind(boost::mem_fn(&FCGIServer::createResponseTimeCounterBlock), this, _1, _2, _3);
    ControlExtension::registerConstructor("response-time", f);
    
    ControlExtension::Constructor f2 =
        boost::bind(boost::mem_fn(&FCGIServer::createResetResponseTimeCounterBlock), this, _1, _2, _3);
    ControlExtension::registerConstructor("reset-response-time", f2);
}

FCGIServer::~FCGIServer() {
}

std::auto_ptr<Block>
FCGIServer::createResponseTimeCounterBlock(const ControlExtension *ext,
                                           Xml *owner,
                                           xmlNodePtr node) {
    return std::auto_ptr<Block>(
        new ResponseTimeCounterBlock(ext, owner, node, responseCounter_));
}

std::auto_ptr<Block>
FCGIServer::createResetResponseTimeCounterBlock(const ControlExtension *ext,
                                                Xml *owner,
                                                xmlNodePtr node) {
    return std::auto_ptr<Block>(
        new ResetResponseTimeCounterBlock(ext, owner, node, responseCounter_));
}

bool
FCGIServer::useXsltProfiler() const {
    return false;
}

void
FCGIServer::run() {

    Config* conf = config();
    
    conf->addForbiddenKey("/xscript/endpoint/*");
    std::string socket = conf->as<std::string>("/xscript/endpoint/socket");
    unsigned short backlog = conf->as<unsigned short>("/xscript/endpoint/backlog");

    conf->addForbiddenKey("/xscript/pidfile");
    pid(conf->as<std::string>("/xscript/pidfile"));

    conf->addForbiddenKey("/xscript/input-buffer");
    conf->addForbiddenKey("/xscript/output-buffer");
    inbuf_size_ = conf->as<unsigned int>("/xscript/input-buffer", 4096);
    outbuf_size_ = conf->as<unsigned int>("/xscript/output-buffer", 4096);

    if (!socket.empty()) {
        socket_ = FCGX_OpenSocket(socket.c_str(), backlog);
        chmod(socket.c_str(), 0666);
    }
    else {
        std::stringstream stream;
        stream << ':' << conf->as<unsigned short>("/xscript/endpoint/port");
        socket_ = FCGX_OpenSocket(stream.str().c_str(), backlog);
    }

    if (-1 == socket_) {
        throw std::runtime_error("can not open fastcgi socket");
    }

    boost::function<void()> f = boost::bind(&FCGIServer::handle, this);
    unsigned short pool_size = conf->as<unsigned short>("/xscript/fastcgi-workers");
    
    conf->stopCollectCache();
    
    workerCounter_->max(pool_size);
    for (unsigned short i = 0; i < pool_size; ++i) {
        create_thread(f);
    }
    join_all();
}

void
FCGIServer::handle() {

    FCGX_Request req;
    std::vector<char> inv(inbuf_size_), outv(outbuf_size_);

    if (0 != FCGX_InitRequest(&req, socket_, 0)) {
        log()->error("can not init fastcgi request");
        return;
    }

    XmlUtils::registerReporters();
    
    for (;;) {
        try {
            boost::shared_ptr<Context> ctx;
            RequestAcceptor request_acceptor(&req);
            if (request_acceptor.accepted()) {
                PROFILER_FORCE(log(), "overall time");
                
                SimpleCounter::ScopedCount c(workerCounter_.get());
                
                fcgi_streambuf inbuf(req.in, &inv[0], inv.size());
                fcgi_streambuf outbuf(req.out, &outv[0], outv.size());

                std::istream is(&inbuf);
                std::ostream os(&outbuf);

                boost::shared_ptr<Request> request(new Request());
                boost::shared_ptr<Response> response(new Response(&os));
                {
                    ResponseDetacher response_detacher(response.get(), ctx);
                    try {
                        request->attach(&is, req.envp);
                        PROFILER_CHECK_POINT("request read and parse");
                        
                        const std::string& script_name = request->getScriptFilename();
                        PROFILER_SET_INFO("overall time for " + script_name);
                        log()->info("requested file: %s", script_name.c_str());
                        
                        handleRequest(request, response, ctx);
                    }
                    catch (const BadRequestError &e) {
                        OperationMode::instance()->sendError(response.get(), 400, e.what());
                    }
                }
                ctx.get() ? responseCounter_->add(ctx.get(), PROFILER_RELEASE()) :
                    responseCounter_->add(response.get(), PROFILER_RELEASE());
            }
        }
        catch (const std::exception &e) {
            log()->error("caught exception while handling request: %s", e.what());
        }
    }
    FCGX_Free(&req, 1);
}

void
FCGIServer::pid(const std::string &file) {
    try {
        std::ofstream f(file.c_str());
        f.exceptions(std::ios::badbit);
        f << static_cast<int>(getpid());
    }
    catch (std::ios::failure &e) {
        std::cerr << "can not open file " << file << std::endl;
        throw;
    }
}

FCGIServer::RequestAcceptor::RequestAcceptor(FCGX_Request *req) : req_(NULL) {
    int result = FCGX_Accept_r(req);
    if (0 == result) {
        req_ = req;
    }
    else {
        log()->error("failed to accept fastcgi request, result: %d", result);
    }
}

FCGIServer::RequestAcceptor::~RequestAcceptor() {
    if (NULL != req_) {
        FCGX_Finish_r(req_);
    }
}

} // namespace xscript
