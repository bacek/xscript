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
#include "server_request.h"
#include "xscript/util.h"
#include "xscript/state.h"
#include "xscript/config.h"
#include "xscript/logger.h"
#include "xscript/script.h"
#include "xscript/writer.h"
#include "xscript/context.h"
#include "xscript/authorizer.h"
#include "xscript/request_data.h"
#include "xscript/vhost_data.h"
#include "xscript/checking_policy.h"
#include "xscript/status_info.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

FCGIServer::FCGIServer(Config *config) :
        Server(config), socket_(-1), inbuf_size_(0), outbuf_size_(0), alternate_port_(0),
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

void
FCGIServer::run() {

    std::string socket = config_->as<std::string>("/xscript/endpoint/socket");
    unsigned short backlog = config_->as<unsigned short>("/xscript/endpoint/backlog");

    pid(config_->as<std::string>("/xscript/pidfile"));

    inbuf_size_ = config_->as<unsigned int>("/xscript/input-buffer", 4096);
    outbuf_size_ = config_->as<unsigned int>("/xscript/output-buffer", 4096);

    alternate_port_ = config_->as<unsigned short>("/xscript/alternate-port", 8080);

    if (!socket.empty()) {
        socket_ = FCGX_OpenSocket(socket.c_str(), backlog);
        chmod(socket.c_str(), 0666);
    }
    else {
        std::stringstream stream;
        stream << ':' << config_->as<unsigned short>("/xscript/endpoint/port");
        socket_ = FCGX_OpenSocket(stream.str().c_str(), backlog);
    }

    if (-1 == socket_) {
        throw std::runtime_error("can not open fastcgi socket");
    }

    boost::function<void()> f = boost::bind(&FCGIServer::handle, this);
    unsigned short pool_size = config_->as<unsigned short>("/xscript/fastcgi-workers");
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

    while (true) {
        if (-1 != FCGX_Accept_r(&req)) {
            try {
                SimpleCounter::ScopedCount c(workerCounter_.get());

                fcgi_streambuf inbuf(req.in, &inv[0], inv.size());
                fcgi_streambuf outbuf(req.out, &outv[0], outv.size());

                std::istream is(&inbuf);
                std::ostream os(&outbuf);

                boost::shared_ptr<RequestResponse> request(new ServerRequest());
                ServerRequest* server_request = dynamic_cast<ServerRequest*>(request.get());

                try {
                    server_request->attach(&is, &os, req.envp);    

                    boost::shared_ptr<RequestData> data(
                        new RequestData(request, boost::shared_ptr<State>(new State())));

                    handleRequest(data);

                    os << std::flush;

                    server_request->detach();
                    FCGX_Finish_r(&req);
                }
                catch (const std::exception &e) {
                    server_request->detach();
                    throw;
                }
            }
            catch (const std::exception &e) {
                FCGX_Finish_r(&req);
                log()->error("caught exception while handling request: %s", e.what());
            }
        }
        else {
            log()->error("failed to accept fastcgi request");
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

bool
FCGIServer::needApplyStylesheet(Request *request) const {
    return (request->getServerPort() != alternate_port_);
}

bool
FCGIServer::fileExists(const std::string &name) const {

    namespace fs = boost::filesystem;
    fs::path path(name);
    return fs::exists(path) && !fs::is_directory(path);
}

} // namespace xscript
