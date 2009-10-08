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
#include "server_response.h"

#include "xscript/authorizer.h"
#include "xscript/config.h"
#include "xscript/context.h"
#include "xscript/logger.h"
#include "xscript/operation_mode.h"
#include "xscript/request_data.h"
#include "xscript/script.h"
#include "xscript/state.h"
#include "xscript/status_info.h"
#include "xscript/vhost_data.h"
#include "xscript/writer.h"
#include "xscript/xml_util.h"

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
        uptimeCounter_() {
    if (0 != FCGX_Init()) {
        throw std::runtime_error("can not init fastcgi library");
    }
    StatusInfo::instance()->getStatBuilder().addCounter(workerCounter_.get());
    StatusInfo::instance()->getStatBuilder().addCounter(&uptimeCounter_);
}

FCGIServer::~FCGIServer() {
}

bool
FCGIServer::useXsltProfiler() const {
    return false;
}

void
FCGIServer::run() {

    Config* conf = config();
    
    std::string socket = conf->as<std::string>("/xscript/endpoint/socket");
    unsigned short backlog = conf->as<unsigned short>("/xscript/endpoint/backlog");

    pid(conf->as<std::string>("/xscript/pidfile"));

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
        RequestAcceptor request_acceptor(&req);
        if (request_acceptor.accepted()) {
            try {
                SimpleCounter::ScopedCount c(workerCounter_.get());

                fcgi_streambuf inbuf(req.in, &inv[0], inv.size());
                fcgi_streambuf outbuf(req.out, &outv[0], outv.size());

                std::istream is(&inbuf);
                std::ostream os(&outbuf);

                boost::shared_ptr<Request> request(new Request());
                boost::shared_ptr<Response> response(new ServerResponse(&os));
                ServerResponse *server_response = dynamic_cast<ServerResponse*>(response.get());
                ResponseDetacher response_detacher(server_response);

                try {
                    request->attach(&is, req.envp);

                    boost::shared_ptr<RequestData> data(
                        new RequestData(request, response, boost::shared_ptr<State>(new State())));

                    handleRequest(data);
                }
                catch (const BadRequestError &e) {
                    OperationMode::sendError(response.get(), 400, e.what());
                    throw;
                }
            }
            catch (const std::exception &e) {
                log()->error("caught exception while handling request: %s", e.what());
            }
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

FCGIServer::ResponseDetacher::ResponseDetacher(ServerResponse *resp) : resp_(resp) {
    assert(NULL != resp);
}

FCGIServer::ResponseDetacher::~ResponseDetacher() {
    resp_->detach();
}


} // namespace xscript
