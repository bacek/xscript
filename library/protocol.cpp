#include "settings.h"

#include <boost/lexical_cast.hpp>

#include <xscript/authorizer.h>
#include <xscript/context.h>
#include <xscript/protocol.h>
#include <xscript/util.h>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

Protocol::MethodMap Protocol::methods_;

class ProtocolRegistrator {
public:
    ProtocolRegistrator();
};

Protocol::Protocol() {
}

Protocol::~Protocol() {
}

std::string
Protocol::getPath(Context *ctx) {
    return ctx->request()->getScriptName();
}

std::string
Protocol::getPathInfo(Context *ctx) {
    return ctx->request()->getPathInfo();
}

std::string
Protocol::getRealPath(Context *ctx) {
    return ctx->request()->getScriptFilename();
}

std::string
Protocol::getOriginalURI(Context *ctx) {
    return ctx->request()->getOriginalURI();
}

std::string
Protocol::getOriginalUrl(Context *ctx) {
    return ctx->request()->getOriginalUrl();
}

std::string
Protocol::getQuery(Context *ctx) {
    return ctx->request()->getQueryString();
}

std::string
Protocol::getRemoteIP(Context *ctx) {
    return ctx->request()->getRealIP();
}

std::string
Protocol::getURI(Context *ctx) {
    return ctx->request()->getURI();
}

std::string
Protocol::getHost(Context *ctx) {
    return ctx->request()->getHost();
}

std::string
Protocol::getOriginalHost(Context *ctx) {
    return ctx->request()->getOriginalHost();
}

std::string
Protocol::getMethod(Context *ctx) {
    return ctx->request()->getRequestMethod();
}

std::string
Protocol::getSecure(Context *ctx) {
    return ctx->request()->isSecure() ? "yes" : "no";
}

std::string
Protocol::getHttpUser(Context *ctx) {
    return ctx->request()->getRemoteUser();
}

std::string
Protocol::getContentLength(Context *ctx) {
    return boost::lexical_cast<std::string>(ctx->request()->getContentLength());
}

std::string
Protocol::getContentEncoding(Context *ctx) {
    return ctx->request()->getContentEncoding();
}

std::string
Protocol::getContentType(Context *ctx) {
    return ctx->request()->getContentType();
}

std::string
Protocol::getBot(Context *ctx) {
    return Authorizer::instance()->checkBot(const_cast<Context*>(ctx)) ? "yes" : "no";
}

std::string
Protocol::get(const Context *ctx, const char* name) {
    return get(const_cast<Context*>(ctx), name);    
}

std::string
Protocol::get(Context *ctx, const char* name) {

    std::string val = StringUtils::tolower(name);

    MethodMap::iterator it = methods_.find(val);
    if (methods_.end() != it) {
        return it->second(ctx);
    }

    return StringUtils::EMPTY_STRING;
}

ProtocolRegistrator::ProtocolRegistrator() {
    Protocol::methods_["path"] = &Protocol::getPath;
    Protocol::methods_["pathinfo"] = &Protocol::getPathInfo;
    Protocol::methods_["realpath"] = &Protocol::getRealPath;
    Protocol::methods_["originaluri"] = &Protocol::getOriginalURI;
    Protocol::methods_["originalurl"] = &Protocol::getOriginalUrl;
    Protocol::methods_["query"] = &Protocol::getQuery;
    Protocol::methods_["remote_ip"] = &Protocol::getRemoteIP;
    Protocol::methods_["uri"] = &Protocol::getURI;
    Protocol::methods_["host"] = &Protocol::getHost;
    Protocol::methods_["originalhost"] = &Protocol::getOriginalHost;
    Protocol::methods_["method"] = &Protocol::getMethod;
    Protocol::methods_["secure"] = &Protocol::getSecure;
    Protocol::methods_["http_user"] = &Protocol::getHttpUser;
    Protocol::methods_["content-length"] = &Protocol::getContentLength;
    Protocol::methods_["content-encoding"] = &Protocol::getContentEncoding;
    Protocol::methods_["content-type"] = &Protocol::getContentType;
    Protocol::methods_["bot"] = &Protocol::getBot;
}

static ProtocolRegistrator reg_;

} // namespace xscript
