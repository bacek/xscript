#ifndef _XSCRIPT_SERVER_H_
#define _XSCRIPT_SERVER_H_

#include <boost/utility.hpp>
#include <string>

#include <xscript/xml_helpers.h>

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

    virtual void run() = 0;

protected:
    virtual void handleRequest(RequestData *request_data);
    virtual bool needApplyStylesheet(Request *request) const;
    static std::pair<std::string, bool> findScript(const std::string &name);
    void sendHeaders(Context *ctx);

    void returnFullPage(Context *ctx, Script *script, XmlDocHelper *doc);
    void cacheFullPage(Context *ctx, Script *script, XmlDocHelper *doc);

protected:
    Config *config_;
};

} // namespace xscript

#endif // _XSCRIPT_SERVER_H_
