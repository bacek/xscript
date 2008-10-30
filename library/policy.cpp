#include "settings.h"

#include <boost/current_function.hpp>

#include "xscript/logger.h"
#include "xscript/policy.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

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
Policy::getProxyHttpHeaders(const Request *req, std::vector<std::string> &headers) {
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

bool
Policy::isSkippedProxyHeader(const std::string &header) {
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
