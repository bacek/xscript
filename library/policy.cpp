#include "settings.h"
#include <cstring>

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
    const char file_scheme[] = "file://";
    const char root_scheme[] = "docroot://";
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

std::string
Policy::getRootByScheme(const Request *request, const std::string &url) {
    const char file_scheme[] = "file://";
    const char root_scheme[] = "docroot://";
    const char http_scheme[] = "http://";
    const char https_scheme[] = "https://";
    if ((strncasecmp(url.c_str(), file_scheme, sizeof(file_scheme) - 1) == 0) ||
        (strncasecmp(url.c_str(), root_scheme, sizeof(root_scheme) - 1) == 0) ||
        (strncasecmp(url.c_str(), http_scheme, sizeof(http_scheme) - 1) == 0) ||
        (strncasecmp(url.c_str(), https_scheme, sizeof(https_scheme) - 1) == 0)) {
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
    return ProxyHeadersHelper::skipped(header.c_str());
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
