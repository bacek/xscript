#include "settings.h"

#include <cctype>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/current_function.hpp>

#include "internal/parser.h"
#include "server_request.h"
#include "xscript/range.h"
#include "xscript/logger.h"
#include "xscript/encoder.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

ServerRequest::ServerRequest() :
    headers_sent_(false), detached_(false), status_(200), stream_(NULL) {
}

ServerRequest::~ServerRequest() {
}

void
ServerRequest::setCookie(const Cookie &cookie) {
    boost::mutex::scoped_lock sl(resp_mutex_);
    if (!headers_sent_) {
        out_cookies_.insert(cookie);
    }
    else {
        throw std::runtime_error("headers already sent");
    }
}

void
ServerRequest::setStatus(unsigned short status) {
    boost::mutex::scoped_lock sl(resp_mutex_);
    if (!headers_sent_) {
        status_ = status;
    }
    else {
        throw std::runtime_error("headers already sent");
    }
}

void
ServerRequest::sendError(unsigned short status, const std::string& message) {
    log()->debug("%s, clearing request output", BOOST_CURRENT_FUNCTION);
    boost::mutex::scoped_lock sl(resp_mutex_);
    if (!headers_sent_) {
        out_cookies_.clear();
        out_headers_.clear();
    }
    else {
        throw std::runtime_error("headers already sent");
    }
    status_ = status;
    out_headers_.insert(std::pair<std::string, std::string>("Content-type", "text/html"));

    boost::mutex::scoped_lock wl(write_mutex_);
    if (detached_) {
        return;
    }
    sendHeadersInternal();

    (*stream_) << "<html><body><h1>" << status << " " << Parser::statusToString(status) << "<br><br>"
    << XmlUtils::escape(createRange(message)) << "</h1></body></html>";
}

void
ServerRequest::setHeader(const std::string &name, const std::string &value) {
    boost::mutex::scoped_lock sl(resp_mutex_);
    if (headers_sent_) {
        throw std::runtime_error("headers already sent");
    }
    std::string normalized_name = Parser::normalizeOutputHeaderName(name);
    std::string::size_type pos = value.find_first_of("\r\n");
    if (pos == std::string::npos) {
        out_headers_[normalized_name] = value;
    }
    else {
        out_headers_[normalized_name].assign(value.begin(), value.begin() + pos);
    }
}

std::streamsize
ServerRequest::write(const char *buf, std::streamsize size) {
    sendHeaders();
    if (!suppressBody()) {
        boost::mutex::scoped_lock wl(write_mutex_);
        if (detached_) {
            return 0;
        }
        stream_->write(buf, size);
    }
    return size;
}

std::string
ServerRequest::outputHeader(const std::string &name) const {
    boost::mutex::scoped_lock sl(resp_mutex_);
    return Parser::get(out_headers_, name);
}

bool
ServerRequest::suppressBody() const {
    return DefaultRequestResponse::suppressBody() || 204 == status_ || 304 == status_;
}

void
ServerRequest::sendHeaders() {
    boost::mutex::scoped_lock sl(resp_mutex_);
    sendHeadersInternal();
}

void
ServerRequest::attach(std::istream *is, std::ostream *os, char *env[]) {
    stream_ = os;
    DefaultRequestResponse::attach(is, env);
    stream_->exceptions(std::ios::badbit);
}

void
ServerRequest::detach() {
    boost::mutex::scoped_lock wl(write_mutex_);
    detached_ = true;
}

void
ServerRequest::sendHeadersInternal() {
    std::stringstream stream;
    if (!headers_sent_) {
        log()->debug("%s, sending headers", BOOST_CURRENT_FUNCTION);
        stream << status_ << " " << Parser::statusToString(status_);
        out_headers_["Status"] = stream.str();
        for (HeaderMap::iterator i = out_headers_.begin(), end = out_headers_.end(); i != end; ++i) {
            (*stream_) << i->first << ": " << i->second << "\r\n";
        }
        for (std::set<Cookie, CookieLess>::const_iterator i = out_cookies_.begin(), end = out_cookies_.end(); i != end; ++i) {
            (*stream_) << "Set-Cookie: " << i->toString() << "\r\n";
        }
        (*stream_) << "\r\n";
        headers_sent_ = true;
    }
}

} // namespace xscript
