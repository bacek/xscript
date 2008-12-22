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

    static std::string get(const Context *ctx, const char *name);

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
    friend class ProtocolRegistrator;
    
    typedef boost::function<std::string (const Context*)> ProtocolMethod;
    typedef std::map<std::string, ProtocolMethod, StringCILess> MethodMap;
    
    static MethodMap methods_;
};

} // namespace xscript
