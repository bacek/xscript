#ifndef _XSCRIPT_DAEMON_SERVER_H_
#define _XSCRIPT_DAEMON_SERVER_H_

#include <string>
#include <boost/thread.hpp>

#include "xscript/server.h"
#include "xscript/simple_counter.h"

#include "uptime_counter.h"

namespace xscript {

class Config;
class Request;
class ServerResponse;

class FCGIServer : public Server, private boost::thread_group {
public:
    FCGIServer(Config *config);
    virtual ~FCGIServer();

    virtual bool useXsltProfiler() const;
    void run();

protected:
    void handle();
    void pid(const std::string &file);

private:
    class RequestAcceptor;

    class ResponseDetacher {
    public:
        explicit ResponseDetacher(ServerResponse *resp);
        ~ResponseDetacher();

    private:
        ServerResponse *resp_;
    };

    int socket_;
    int inbuf_size_, outbuf_size_;

    std::auto_ptr<SimpleCounter> workerCounter_;
    UptimeCounter uptimeCounter_;
};

} // namespace xscript

#endif // _XSCRIPT_DAEMON_SERVER_H_
