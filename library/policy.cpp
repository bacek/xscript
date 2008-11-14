#include "settings.h"
#include <cstring>

#include <boost/current_function.hpp>

#include "xscript/logger.h"
#include "xscript/policy.h"
#include "xscript/request.h"
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

void
Policy::getProxyHttpHeaders(const Request *req, std::vector<std::string> &headers) const {
    headers.clear();
    if (req->countHeaders() > 0) {
        std::vector<std::string> names;
        req->headerNames(names);
        for (std::vector<std::string>::const_iterator i = names.begin(), end = names.end(); i != end; ++i) {
            const std::string& name = *i;
            const std::string& value = req->getHeader(name);
            if (isSkippedProxyHeader(name)) {
                log()->debug("%s, skipped %s: %s", BOOST_CURRENT_FUNCTION, name.c_str(), value.c_str());
            }
            else {
                headers.push_back(name);
                headers.back().append(": ").append(value);
            }
        }
    }
}

std::string
Policy::getPathByScheme(const std::string &url) const {

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
        return VirtualHostData::instance()->getDocumentRoot(NULL) + (url.c_str() + pos);
    }

    return url;
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

bool
Policy::isSkippedProxyHeader(const std::string &header) const {
    return ProxyHeadersHelper::skipped(header.c_str());
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

REGISTER_COMPONENT(Policy);

} // namespace xscript
