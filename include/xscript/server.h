#ifndef _XSCRIPT_SERVER_H_
#define _XSCRIPT_SERVER_H_

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <string>

namespace xscript {

class Config;
class RequestData;
class Request;
class Context;

class Server : private boost::noncopyable {
public:
    Server(Config *config);
    virtual ~Server();

    virtual void run() = 0;

protected:
    virtual void handleRequest(const boost::shared_ptr<RequestData>& request_data);
    virtual bool needApplyStylesheet(Request *request) const;
    static std::pair<std::string, bool> findScript(const std::string &name);
    void sendHeaders(Context *ctx);

protected:
    Config *config_;
};

} // namespace xscript

#endif // _XSCRIPT_SERVER_H_
