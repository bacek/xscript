#include "settings.h"

#include <map>
#include <string>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <xscript/functors.h>

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

    static const std::string& getPathNative(const Context *ctx);
    static const std::string& getPathInfoNative(const Context *ctx);
    static const std::string& getRealPathNative(const Context *ctx);
    static const std::string& getQueryNative(const Context *ctx);
    static const std::string& getRemoteIPNative(const Context *ctx);
    static const std::string& getMethodNative(const Context *ctx);
    static const std::string& getHttpUserNative(const Context *ctx);
    static const std::string& getContentEncodingNative(const Context *ctx);
    static const std::string& getContentTypeNative(const Context *ctx);

    static std::string get(const Context *ctx, const char* name);

    static const std::string PATH;
    static const std::string PATH_INFO;
    static const std::string REAL_PATH;
    static const std::string ORIGINAL_URI;
    static const std::string ORIGINAL_URL;
    static const std::string QUERY;
    static const std::string REMOTE_IP;
    static const std::string URI;
    static const std::string HOST;
    static const std::string ORIGINAL_HOST;
    static const std::string METHOD;
    static const std::string SECURE;
    static const std::string HTTP_USER;
    static const std::string CONTENT_LENGTH;
    static const std::string CONTENT_ENCODING;
    static const std::string CONTENT_TYPE;
    static const std::string BOT;

private:
    typedef boost::function<std::string (const Context*)> ProtocolMethod;
    friend class ProtocolRegistrator;

private:
    typedef std::map<std::string, ProtocolMethod, StringCILess> MethodMap;
    static MethodMap methods_;
};

} // namespace xscript
