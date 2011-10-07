#include "settings.h"
#include <cstring>
#include <set>

#include <boost/current_function.hpp>

#include "xscript/context.h"
#include "xscript/logger.h"
#include "xscript/policy.h"
#include "xscript/request.h"
#include "xscript/string_utils.h"
#include "xscript/util.h"
#include "xscript/vhost_data.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

static std::string UTF8_ENCODING = "utf-8";

static const char file_scheme[] = "file://";
static const char root_scheme[] = "docroot://";
static const char http_scheme[] = "http://";
static const char https_scheme[] = "https://";


class ProxyHeadersChecker {
public:
    ProxyHeadersChecker();

    bool skipped(const char *name) const {
        return disallow_proxy_headers_.find(name) != disallow_proxy_headers_.end();
    }

private:
    struct StringRawPtrLess {
        bool operator() (const char *val, const char *target) const {
            return strcasecmp(val, target) < 0;
        }
    };

    std::set<const char*, StringRawPtrLess> disallow_proxy_headers_;
};					    

ProxyHeadersChecker::ProxyHeadersChecker() {
    disallow_proxy_headers_.insert("host");
    disallow_proxy_headers_.insert("if-modified-since");
    disallow_proxy_headers_.insert("accept-encoding");
    disallow_proxy_headers_.insert("keep-alive");
    disallow_proxy_headers_.insert("connection");
    disallow_proxy_headers_.insert("content-length");
}

static ProxyHeadersChecker proxy_headers_checker_;


Policy::Policy()
{}

Policy::~Policy()
{}

const std::string&
Policy::realIPHeaderName() {
    return StringUtils::EMPTY_STRING;
}

std::string
Policy::getPathByScheme(const Request *request, const std::string &url) {
    if (strncasecmp(url.c_str(), file_scheme, sizeof(file_scheme) - 1) == 0) {
        return url.substr(sizeof(file_scheme) - 1);
    }
    else if (strncasecmp(url.c_str(), root_scheme, sizeof(root_scheme) - 1) == 0) {
        std::string::size_type pos = sizeof(root_scheme) - 1;
        if ('/' != url[pos]) {
            --pos;
        }
        return VirtualHostData::instance()->getDocumentRoot(request) + (url.c_str() + pos);
    }
    return url;
}

bool
Policy::isRemoteScheme(const char *url) {
    return !strncasecmp(url, http_scheme, sizeof(http_scheme) - 1) ||
           !strncasecmp(url, https_scheme, sizeof(https_scheme) - 1);
}

std::string
Policy::getRootByScheme(const Request *request, const std::string &url) {
    if (!strncasecmp(url.c_str(), file_scheme, sizeof(file_scheme) - 1) ||
        !strncasecmp(url.c_str(), root_scheme, sizeof(root_scheme) - 1) ||
	isRemoteScheme(url.c_str())) {

        return VirtualHostData::instance()->getDocumentRoot(request);
    }
    return StringUtils::EMPTY_STRING;
}

std::string
Policy::getKey(const Request *request, const std::string &name) {
    (void)request;
    return name;
}

std::string
Policy::getOutputEncoding(const Request *request) {
    (void)request;
    return UTF8_ENCODING;
}

void
Policy::useDefaultSanitizer() {
}

bool
Policy::isSkippedProxyHeader(const std::string &header) {
    return proxy_headers_checker_.skipped(header.c_str());
}

void
Policy::processCacheLevel(TaggedBlock *block, const std::string &no_cache) {
    (void)block;
    (void)no_cache;
}

bool
Policy::allowCaching(const Context *ctx, const TaggedBlock *block) {
    (void)ctx;
    (void)block;
    return true;
}

bool
Policy::allowCachingInputCookie(const char *name) {
    (void)name;
    return true;
}

bool
Policy::allowCachingOutputCookie(const char *name) {
    (void)name;
    return false;
}

bool
Policy::isErrorDoc(xmlDocPtr doc) {
    (void)doc;
    return false;
}

static ComponentRegisterer<Policy> reg;

} // namespace xscript
