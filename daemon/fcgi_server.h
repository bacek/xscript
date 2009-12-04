#ifndef _XSCRIPT_DAEMON_SERVER_H_
#define _XSCRIPT_DAEMON_SERVER_H_

#include <string>
#include <boost/thread.hpp>

#include <libxml/tree.h>

#include "xscript/block.h"
#include "xscript/response_time_counter.h"
#include "xscript/server.h"
#include "xscript/simple_counter.h"

#include "uptime_counter.h"

namespace xscript {

class Config;
class ControlExtension;
class Request;
class ServerResponse;
class Xml;

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
    std::auto_ptr<Block>
    createResponseTimeCounterBlock(const ControlExtension *ext, Xml *owner, xmlNodePtr node);

    std::auto_ptr<Block>
    createResetResponseTimeCounterBlock(const ControlExtension *ext, Xml *owner, xmlNodePtr node);
    
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
    boost::shared_ptr<ResponseTimeCounter> responseCounter_;
};

} // namespace xscript

#endif // _XSCRIPT_DAEMON_SERVER_H_
