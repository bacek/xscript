#ifndef _XSCRIPT_SERVER_H_
#define _XSCRIPT_SERVER_H_

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <string>

#include "xscript/xml_helpers.h"

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
    virtual unsigned short alternatePort() const;
    virtual unsigned short noXsltPort() const;
    virtual bool useXsltProfiler() const = 0;
    const std::string& hostname() const;

protected:
    virtual void handleRequest(const boost::shared_ptr<RequestData>& request_data);    
    bool processCachedDoc(Context *ctx, const Script *script);
    void sendResponse(Context *ctx, XmlDocHelper doc);
    void sendResponseCached(Context *ctx, const Script *script, XmlDocHelper doc);
                               
    virtual boost::shared_ptr<Script> getScript(Request *request);
    static std::pair<std::string, bool> findScript(const std::string &name);
    void addHeaders(Context *ctx);
    void sendHeaders(Context *ctx);

    virtual Context* createContext(const boost::shared_ptr<Script> &script,
                                   const boost::shared_ptr<RequestData> &request_data);

    Config* config() const;
private:
    class ServerData;
    ServerData *data_;
};

} // namespace xscript

#endif // _XSCRIPT_SERVER_H_
