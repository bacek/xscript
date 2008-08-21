#ifndef _XSCRIPT_SERVER_REQUEST_H_
#define _XSCRIPT_SERVER_REQUEST_H_

#include <set>
#include <map>
#include <iosfwd>
#include <functional>
#include <boost/cstdint.hpp>
#include <boost/thread/mutex.hpp>

#include "settings.h"

#include "xscript/functors.h"
#include "xscript/util.h"
#include "xscript/range.h"
#include "xscript/cookie.h"
#include "details/default_request_response.h"

namespace xscript {

class ServerRequest : public DefaultRequestResponse {
public:
    ServerRequest();
    virtual ~ServerRequest();

    virtual void setCookie(const Cookie &cookie);
    virtual void setStatus(unsigned short status);
    virtual void sendError(unsigned short status, const std::string& message);
    virtual void setHeader(const std::string &name, const std::string &value);
    virtual std::streamsize write(const char *buf, std::streamsize size);
    virtual std::string outputHeader(const std::string &name) const;
    virtual bool suppressBody() const;
    virtual void sendHeaders();
    virtual void attach(std::istream *is, std::ostream *os, char *env[]);
    virtual void detach();

private:
    void sendHeadersInternal();

private:
    bool headers_sent_, detached_;
    unsigned short status_;

    std::ostream *stream_;
    mutable boost::mutex resp_mutex_, write_mutex_;

    HeaderMap out_headers_;
    std::set<Cookie, CookieLess> out_cookies_;
};

} // namespace xscript

#endif // _XSCRIPT_SERVER_REQUEST_H_
