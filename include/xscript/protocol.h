#include "settings.h"

#include <map>
#include <string>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class Context;
class ProtocolRegistrator;

class Protocol : public boost::noncopyable {
public:
    Protocol();
    virtual ~Protocol();

    static std::string getPath(const Context *ctx);
    static std::string getPathInfo(const Context *ctx);
    static std::string getRealPath(const Context *ctx);
    static std::string getOriginalURI(const Context *ctx);
    static std::string getOriginalUrl(const Context *ctx);
    static std::string getQuery(const Context *ctx);
    static std::string getRemoteIP(const Context *ctx);
    static std::string getURI(const Context *ctx);
    static std::string getHost(const Context *ctx);
    static std::string getOriginalHost(const Context *ctx);
    static std::string getMethod(const Context *ctx);
    static std::string getSecure(const Context *ctx);
    static std::string getHttpUser(const Context *ctx);
    static std::string getContentLength(const Context *ctx);
    static std::string getContentEncoding(const Context *ctx);
    static std::string getContentType(const Context *ctx);
    static std::string getBot(const Context *ctx);

    static std::string get(const Context *ctx, const char* name);

private:
    typedef boost::function<std::string (const Context*)> ProtocolMethod;
    friend class ProtocolRegistrator;

private:
    typedef std::map<std::string, ProtocolMethod> MethodMap;
    static MethodMap methods_;
};

} // namespace xscript
