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
class Script;

class Server : private boost::noncopyable {
public:
    Server(Config *config);
    virtual ~Server();

    virtual bool needApplyMainStylesheet(Request *request) const;
    virtual bool needApplyPerblockStylesheet(Request *request) const;
    virtual std::streamsize maxBodyLength(Request *request) const;
    virtual unsigned short alternatePort() const;
    virtual unsigned short noXsltPort() const;
    virtual bool useXsltProfiler() const = 0;
    const std::string& hostname() const;

protected:
    virtual void handleRequest(const boost::shared_ptr<RequestData>& request_data);
    virtual boost::shared_ptr<Script> getScript(const std::string &name, Request *request);
    static std::pair<std::string, bool> findScript(const std::string &name);
    void sendHeaders(Context *ctx);

    virtual Context* createContext(
        const boost::shared_ptr<Script> &script, const boost::shared_ptr<RequestData> &request_data);

protected:
    Config *config_;
    unsigned short alternate_port_, noxslt_port_;

private:
    std::string hostname_;
    std::streamsize max_body_length_;
    
    // todo: move to VirtualHostData
    static const std::string MAX_BODY_LENGTH;
};

} // namespace xscript

#endif // _XSCRIPT_SERVER_H_
