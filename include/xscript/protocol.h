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

    static std::string getPath(Context *ctx);
    static std::string getPathInfo(Context *ctx);
    static std::string getRealPath(Context *ctx);
    static std::string getOriginalURI(Context *ctx);
    static std::string getOriginalUrl(Context *ctx);
    static std::string getQuery(Context *ctx);
    static std::string getRemoteIP(Context *ctx);
    static std::string getURI(Context *ctx);
    static std::string getHost(Context *ctx);
    static std::string getOriginalHost(Context *ctx);
    static std::string getMethod(Context *ctx);
    static std::string getSecure(Context *ctx);
    static std::string getHttpUser(Context *ctx);
    static std::string getContentLength(Context *ctx);
    static std::string getContentEncoding(Context *ctx);
    static std::string getContentType(Context *ctx);
    static std::string getBot(Context *ctx);

    static std::string get(const Context *ctx, const char* name);
    static std::string get(Context *ctx, const char* name);

private:
    typedef boost::function<std::string (Context*)> ProtocolMethod;
    friend class ProtocolRegistrator;

private:
    typedef std::map<std::string, ProtocolMethod> MethodMap;
    static MethodMap methods_;
};

} // namespace xscript
