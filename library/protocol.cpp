#include "settings.h"

#include <cassert>

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

const std::string Protocol::PATH = "path";
const std::string Protocol::PATH_INFO = "pathinfo";
const std::string Protocol::REAL_PATH = "realpath";
const std::string Protocol::ORIGINAL_URI = "originaluri";
const std::string Protocol::ORIGINAL_URL = "originalurl";
const std::string Protocol::QUERY = "query";
const std::string Protocol::REMOTE_IP = "remote_ip";
const std::string Protocol::URI = "uri";
const std::string Protocol::HOST = "host";
const std::string Protocol::ORIGINAL_HOST = "originalhost";
const std::string Protocol::METHOD = "method";
const std::string Protocol::SECURE = "secure";
const std::string Protocol::HTTP_USER = "http_user";
const std::string Protocol::CONTENT_LENGTH = "content-length";
const std::string Protocol::CONTENT_ENCODING = "content-encoding";
const std::string Protocol::CONTENT_TYPE = "content-type";
const std::string Protocol::BOT = "bot";

class ProtocolRegistrator {
public:
    ProtocolRegistrator();
};

Protocol::Protocol() {
}

Protocol::~Protocol() {
}

std::string
Protocol::getPath(const Context *ctx) {
    return getPathNative(ctx);
}

std::string
Protocol::getPathInfo(const Context *ctx) {
    return getPathInfoNative(ctx);
}

std::string
Protocol::getRealPath(const Context *ctx) {
    return getRealPathNative(ctx);
}

std::string
Protocol::getOriginalURI(const Context *ctx) {
    return ctx->request()->getOriginalURI();
}

std::string
Protocol::getOriginalUrl(const Context *ctx) {
    return ctx->request()->getOriginalUrl();
}

std::string
Protocol::getQuery(const Context *ctx) {
    return getQueryNative(ctx);
}

std::string
Protocol::getRemoteIP(const Context *ctx) {
    return getRemoteIPNative(ctx);
}

std::string
Protocol::getURI(const Context *ctx) {
    return ctx->request()->getURI();
}

std::string
Protocol::getHost(const Context *ctx) {
    return ctx->request()->getHost();
}

std::string
Protocol::getOriginalHost(const Context *ctx) {
    return ctx->request()->getOriginalHost();
}

std::string
Protocol::getMethod(const Context *ctx) {
    return getMethodNative(ctx);
}

std::string
Protocol::getSecure(const Context *ctx) {
    return ctx->request()->isSecure() ? "yes" : "no";
}

std::string
Protocol::getHttpUser(const Context *ctx) {
    return getHttpUserNative(ctx);
}

std::string
Protocol::getContentLength(const Context *ctx) {
    return boost::lexical_cast<std::string>(ctx->request()->getContentLength());
}

std::string
Protocol::getContentEncoding(const Context *ctx) {
    return getContentEncodingNative(ctx);
}

std::string
Protocol::getContentType(const Context *ctx) {
    return getContentTypeNative(ctx);
}

std::string
Protocol::getBot(const Context *ctx) {
    return Authorizer::instance()->checkBot(const_cast<Context*>(ctx)) ? "yes" : "no";
}

const std::string&
Protocol::getPathNative(const Context *ctx) {
    return ctx->request()->getScriptName();
}

const std::string&
Protocol::getPathInfoNative(const Context *ctx) {
    return ctx->request()->getPathInfo();
}

const std::string&
Protocol::getRealPathNative(const Context *ctx) {
    return ctx->request()->getScriptFilename();
}

const std::string&
Protocol::getQueryNative(const Context *ctx) {
    return ctx->request()->getQueryString();
}

const std::string&
Protocol::getRemoteIPNative(const Context *ctx) {
    return ctx->request()->getRealIP();
}

const std::string&
Protocol::getMethodNative(const Context *ctx) {
    return ctx->request()->getRequestMethod();
}

const std::string&
Protocol::getHttpUserNative(const Context *ctx) {
    return ctx->request()->getRemoteUser();
}

const std::string&
Protocol::getContentEncodingNative(const Context *ctx) {
    return ctx->request()->getContentEncoding();
}

const std::string&
Protocol::getContentTypeNative(const Context *ctx) {
    return ctx->request()->getContentType();
}

std::string
Protocol::get(const Context *ctx, const char* name) {
    assert(ctx);
    MethodMap::iterator it = methods_.find(name);
    if (methods_.end() == it) {
        throw std::runtime_error(std::string("Unknown protocol arg: ") + name);
    }

    return it->second(ctx);
}

ProtocolRegistrator::ProtocolRegistrator() {
    Protocol::methods_[Protocol::PATH] = &Protocol::getPath;
    Protocol::methods_[Protocol::PATH_INFO] = &Protocol::getPathInfo;
    Protocol::methods_[Protocol::REAL_PATH] = &Protocol::getRealPath;
    Protocol::methods_[Protocol::ORIGINAL_URI] = &Protocol::getOriginalURI;
    Protocol::methods_[Protocol::ORIGINAL_URL] = &Protocol::getOriginalUrl;
    Protocol::methods_[Protocol::QUERY] = &Protocol::getQuery;
    Protocol::methods_[Protocol::REMOTE_IP] = &Protocol::getRemoteIP;
    Protocol::methods_[Protocol::URI] = &Protocol::getURI;
    Protocol::methods_[Protocol::HOST] = &Protocol::getHost;
    Protocol::methods_[Protocol::ORIGINAL_HOST] = &Protocol::getOriginalHost;
    Protocol::methods_[Protocol::METHOD] = &Protocol::getMethod;
    Protocol::methods_[Protocol::SECURE] = &Protocol::getSecure;
    Protocol::methods_[Protocol::HTTP_USER] = &Protocol::getHttpUser;
    Protocol::methods_[Protocol::CONTENT_LENGTH] = &Protocol::getContentLength;
    Protocol::methods_[Protocol::CONTENT_ENCODING] = &Protocol::getContentEncoding;
    Protocol::methods_[Protocol::CONTENT_TYPE] = &Protocol::getContentType;
    Protocol::methods_[Protocol::BOT] = &Protocol::getBot;
}

static ProtocolRegistrator reg_;

} // namespace xscript
