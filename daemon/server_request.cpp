#include "settings.h"

#include <cctype>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/current_function.hpp>

#include "details/parser.h"
#include "server_request.h"
#include "xscript/range.h"
#include "xscript/logger.h"
#include "xscript/encoder.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

ServerRequest::ServerRequest() :
        impl_(RequestFactory::instance()->create()) {
    reset();
}

ServerRequest::~ServerRequest() {
}

unsigned short
ServerRequest::getServerPort() const {
    return impl_->getServerPort();
}

const std::string&
ServerRequest::getServerAddr() const {
    return impl_->getServerAddr();
}

const std::string&
ServerRequest::getPathInfo() const {
    return impl_->getPathInfo();
}

const std::string&
ServerRequest::getPathTranslated() const {
    return impl_->getPathTranslated();
}

const std::string&
ServerRequest::getScriptName() const {
    return impl_->getScriptName();
}

const std::string&
ServerRequest::getScriptFilename() const {
    return impl_->getScriptFilename();
}

const std::string&
ServerRequest::getDocumentRoot() const {
    return impl_->getDocumentRoot();
}

const std::string&
ServerRequest::getRemoteUser() const {
    return impl_->getRemoteUser();
}

const std::string&
ServerRequest::getRemoteAddr() const {
    return impl_->getRemoteAddr();
}

const std::string&
ServerRequest::getRealIP() const {
    return impl_->getRealIP();
}

const std::string&
ServerRequest::getQueryString() const {
    return impl_->getQueryString();
}

const std::string&
ServerRequest::getRequestMethod() const {
    return impl_->getRequestMethod();
}

std::string
ServerRequest::getURI() const {
    return impl_->getURI();
}

std::string
ServerRequest::getOriginalURI() const {
    boost::mutex::scoped_lock sl(mutex_);
    return impl_->getOriginalURI();
}

std::string
ServerRequest::getHost() const {
    boost::mutex::scoped_lock sl(mutex_);
    return impl_->getHost();
}

std::string
ServerRequest::getOriginalHost() const {
    boost::mutex::scoped_lock sl(mutex_);
    return impl_->getOriginalHost();
}

std::string
ServerRequest::getOriginalUrl() const {
    boost::mutex::scoped_lock sl(mutex_);
    return impl_->getOriginalUrl();
}

std::streamsize
ServerRequest::getContentLength() const {
    boost::mutex::scoped_lock sl(mutex_);
    return impl_->getContentLength();
}

const std::string&
ServerRequest::getContentType() const {
    boost::mutex::scoped_lock sl(mutex_);
    return impl_->getContentType();
}

const std::string&
ServerRequest::getContentEncoding() const {
    boost::mutex::scoped_lock sl(mutex_);
    return impl_->getContentEncoding();
}

unsigned int
ServerRequest::countArgs() const {
    return impl_->countArgs();
}

bool
ServerRequest::hasArg(const std::string &name) const {
    return impl_->hasArg(name);
}

const std::string&
ServerRequest::getArg(const std::string &name) const {
    return impl_->getArg(name);
}

void
ServerRequest::getArg(const std::string &name, std::vector<std::string> &v) const {
    impl_->getArg(name, v);
}

void
ServerRequest::argNames(std::vector<std::string> &v) const {
    impl_->argNames(v);
}

unsigned int
ServerRequest::countHeaders() const {
    boost::mutex::scoped_lock sl(mutex_);
    return impl_->countHeaders();
}

bool
ServerRequest::hasHeader(const std::string &name) const {
    boost::mutex::scoped_lock sl(mutex_);
    return impl_->hasHeader(name);
}

const std::string&
ServerRequest::getHeader(const std::string &name) const {
    boost::mutex::scoped_lock sl(mutex_);
    return impl_->getHeader(name);
}

void
ServerRequest::headerNames(std::vector<std::string> &v) const {
    boost::mutex::scoped_lock sl(mutex_);
    impl_->headerNames(v);
}

unsigned int
ServerRequest::countCookies() const {
    return impl_->countCookies();
}

bool
ServerRequest::hasCookie(const std::string &name) const {
    return impl_->hasCookie(name);
}

const std::string&
ServerRequest::getCookie(const std::string &name) const {
    return impl_->getCookie(name);
}

void
ServerRequest::cookieNames(std::vector<std::string> &v) const {
    impl_->cookieNames(v);
}

unsigned int
ServerRequest::countVariables() const {
    return impl_->countVariables();
}

bool
ServerRequest::hasVariable(const std::string &name) const {
    return impl_->hasVariable(name);
}

const std::string&
ServerRequest::getVariable(const std::string &name) const {
    return impl_->getVariable(name);
}

void
ServerRequest::variableNames(std::vector<std::string> &v) const {
    impl_->variableNames(v);
}

bool
ServerRequest::hasFile(const std::string &name) const {
    return impl_->hasFile(name);
}

const std::string&
ServerRequest::remoteFileName(const std::string &name) const {
    return impl_->remoteFileName(name);
}

const std::string&
ServerRequest::remoteFileType(const std::string &name) const {
    return impl_->remoteFileType(name);
}

std::pair<const char*, std::streamsize>
ServerRequest::remoteFile(const std::string &name) const {
    return impl_->remoteFile(name);
}

bool
ServerRequest::isSecure() const {
    return impl_->isSecure();
}

std::pair<const char*, std::streamsize>
ServerRequest::requestBody() const {
    return impl_->requestBody();
}

void
ServerRequest::setCookie(const Cookie &cookie) {
    boost::mutex::scoped_lock sl(mutex_);
    if (!headers_sent_) {
        out_cookies_.insert(cookie);
    }
    else {
        throw std::runtime_error("headers already sent");
    }
}

void
ServerRequest::setStatus(unsigned short status) {
    boost::mutex::scoped_lock sl(mutex_);
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
    boost::mutex::scoped_lock sl(mutex_);
    if (!headers_sent_) {
        out_cookies_.clear();
        out_headers_.clear();
    }
    else {
        throw std::runtime_error("headers already sent");
    }
    status_ = status;
    out_headers_.insert(std::pair<std::string, std::string>("Content-type", "text/html"));
    sendHeadersInternal();

    (*stream_) << "<html><body><h1>" << status << " " << Parser::statusToString(status) << "<br><br>"
    << XmlUtils::escape(createRange(message)) << "</h1></body></html>";
}


void
ServerRequest::setHeader(const std::string &name, const std::string &value) {
    boost::mutex::scoped_lock sl(mutex_);
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
        stream_->write(buf, size);
    }
    return size;
}

std::string
ServerRequest::outputHeader(const std::string &name) const {
    boost::mutex::scoped_lock sl(mutex_);
    return Parser::get(out_headers_, name);
}

bool
ServerRequest::suppressBody() const {
    return impl_->suppressBody() || 204 == status_ || 304 == status_;
}

void
ServerRequest::reset() {

    impl_->reset();

    status_ = 200;
    stream_ = NULL;
    headers_sent_ = false;
    out_cookies_.clear();
    out_headers_.clear();
}

void
ServerRequest::sendHeaders() {
    boost::mutex::scoped_lock sl(mutex_);
    sendHeadersInternal();

}

void
ServerRequest::attach(std::istream *is, std::ostream *os, char *env[]) {

    stream_ = os;
    impl_->attach(is, env);
    stream_->exceptions(std::ios::badbit);
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

void
ServerRequest::addInputHeader(const std::string &name, const std::string &value) {
    boost::mutex::scoped_lock sl(mutex_);
    impl_->addInputHeader(name, value);
}

} // namespace xscript
