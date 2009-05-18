#include "settings.h"
#include <cstring>

#include <boost/current_function.hpp>

#include "xscript/logger.h"
#include "xscript/policy.h"
#include "xscript/request.h"
#include "xscript/util.h"
#include "xscript/string_utils.h"
#include "xscript/vhost_data.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

const std::string Policy::UTF8_ENCODING = "utf-8";

class ProxyHeadersHelper {
public:
    static bool skipped(const char* name);

private:
    struct StrSize {
        const char* name;
        int size;
    };

    static const StrSize skipped_headers[];
};

Policy::Policy() {
}

Policy::~Policy() {
}

std::string
Policy::getPathByScheme(const Request *request, const std::string &url) const {

    const char file_scheme[] = "file://";
    if (!strncasecmp(url.c_str(), file_scheme, sizeof(file_scheme) - 1)) {
        return url.substr(sizeof(file_scheme) - 1);
    }

    const char root_scheme[] = "docroot://";
    if (!strncasecmp(url.c_str(), root_scheme, sizeof(root_scheme) - 1)) {
        std::string::size_type pos = sizeof(root_scheme) - 1;
        if ('/' != url[pos]) {
            --pos;
        }
        return VirtualHostData::instance()->getDocumentRoot(request) + (url.c_str() + pos);
    }

    return url;
}

std::string
Policy::getRootByScheme(const Request *request, const std::string &url) const {

    const char file_scheme[] = "file://";
    const char root_scheme[] = "docroot://";
    const char http_scheme[] = "http://";
    const char https_scheme[] = "https://";

    if ((!strncasecmp(url.c_str(), file_scheme, sizeof(file_scheme) - 1)) ||
        (!strncasecmp(url.c_str(), root_scheme, sizeof(root_scheme) - 1)) ||
        (!strncasecmp(url.c_str(), http_scheme, sizeof(http_scheme) - 1)) ||
        (!strncasecmp(url.c_str(), https_scheme, sizeof(https_scheme) - 1))) {
        return VirtualHostData::instance()->getDocumentRoot(request);
    }

    return StringUtils::EMPTY_STRING;
}

std::string
Policy::getKey(const Request* request, const std::string& name) const {
    (void)request;
    return name;
}

std::string
Policy::getOutputEncoding(const Request* request) const {
    (void)request;
    return UTF8_ENCODING;
}

void
Policy::useDefaultSanitizer() const {
}

bool
Policy::isSkippedProxyHeader(const std::string &header) const {
    return ProxyHeadersHelper::skipped(header.c_str());
}

void
Policy::processCacheLevel(TaggedBlock *block, const std::string &no_cache) const {
    (void)block;
    (void)no_cache;
}

bool
Policy::allowCaching(const Context *ctx, const TaggedBlock *block) const {
    (void)ctx;
    (void)block;
    return true;
}

bool
Policy::allowCachingCookie(const char *name) const {
    (void)name;
    return false;
}

const std::string&
Policy::realIPHeaderName() const {
    return StringUtils::EMPTY_STRING;
}

bool
Policy::isErrorDoc(xmlDocPtr doc) const {
    (void)doc;
    return false;
}

const ProxyHeadersHelper::StrSize
ProxyHeadersHelper::skipped_headers[] = {
    { "host", sizeof("host") },
    { "if-modified-since", sizeof("if-modified-since") },
    { "accept-encoding", sizeof("accept-encoding") },
    { "keep-alive", sizeof("keep-alive") },
    { "connection", sizeof("connection") },
    { "content-length", sizeof("content-length") },
};

bool
ProxyHeadersHelper::skipped(const char* name) {
    for ( int i = sizeof(skipped_headers) / sizeof(skipped_headers[0]); i--; ) {
        const StrSize& sh = skipped_headers[i];
        if (strncasecmp(name, sh.name, sh.size) == 0) {
            return true;
        }
    }

    return false;
}

static ComponentRegisterer<Policy> reg;

} // namespace xscript
