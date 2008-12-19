#include "settings.h"

#include <iostream>

#include <boost/current_function.hpp>
#include <boost/lexical_cast.hpp>

#include "internal/default_request_response.h"
#include "internal/parser.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

DefaultRequestResponse::DefaultRequestResponse() :
    impl_(RequestFactory::instance()->create()), writer_(NULL),
    headers_sent_(false), status_(200), detached_(false), stream_locked_(false) {
}

DefaultRequestResponse::~DefaultRequestResponse() {
}

unsigned short
DefaultRequestResponse::getServerPort() const {
    return impl_->getServerPort();
}

const std::string&
DefaultRequestResponse::getServerAddr() const {
    return impl_->getServerAddr();
}

const std::string&
DefaultRequestResponse::getPathInfo() const {
    return impl_->getPathInfo();
}

const std::string&
DefaultRequestResponse::getPathTranslated() const {
    return impl_->getPathTranslated();
}

const std::string&
DefaultRequestResponse::getScriptName() const {
    return impl_->getScriptName();
}

const std::string&
DefaultRequestResponse::getScriptFilename() const {
    return impl_->getScriptFilename();
}

const std::string&
DefaultRequestResponse::getDocumentRoot() const {
    return impl_->getDocumentRoot();
}

const std::string&
DefaultRequestResponse::getRemoteUser() const {
    return impl_->getRemoteUser();
}

const std::string&
DefaultRequestResponse::getRemoteAddr() const {
    return impl_->getRemoteAddr();
}

const std::string&
DefaultRequestResponse::getRealIP() const {
    return impl_->getRealIP();
}

const std::string&
DefaultRequestResponse::getQueryString() const {
    return impl_->getQueryString();
}

const std::string&
DefaultRequestResponse::getRequestMethod() const {
    return impl_->getRequestMethod();
}

std::string
DefaultRequestResponse::getURI() const {
    return impl_->getURI();
}

std::string
DefaultRequestResponse::getOriginalURI() const {
    return impl_->getOriginalURI();
}

std::string
DefaultRequestResponse::getHost() const {
    return impl_->getHost();
}

std::string
DefaultRequestResponse::getOriginalHost() const {
    return impl_->getOriginalHost();
}

std::string
DefaultRequestResponse::getOriginalUrl() const {
    return impl_->getOriginalUrl();
}

std::streamsize
DefaultRequestResponse::getContentLength() const {
    return impl_->getContentLength();
}

const std::string&
DefaultRequestResponse::getContentType() const {
    return impl_->getContentType();
}

const std::string&
DefaultRequestResponse::getContentEncoding() const {
    return impl_->getContentEncoding();
}

unsigned int
DefaultRequestResponse::countArgs() const {
    return impl_->countArgs();
}

bool
DefaultRequestResponse::hasArg(const std::string &name) const {
    return impl_->hasArg(name);
}

const std::string&
DefaultRequestResponse::getArg(const std::string &name) const {
    return impl_->getArg(name);
}

void
DefaultRequestResponse::getArg(const std::string &name, std::vector<std::string> &v) const {
    impl_->getArg(name, v);
}

void
DefaultRequestResponse::argNames(std::vector<std::string> &v) const {
    impl_->argNames(v);
}

void
DefaultRequestResponse::setArg(const std::string &name, const std::string &value) {
    impl_->setArg(name, value);
}

unsigned int
DefaultRequestResponse::countHeaders() const {
    return impl_->countHeaders();
}

bool
DefaultRequestResponse::hasHeader(const std::string &name) const {
    return impl_->hasHeader(name);
}

const std::string&
DefaultRequestResponse::getHeader(const std::string &name) const {
    return impl_->getHeader(name);
}

void
DefaultRequestResponse::headerNames(std::vector<std::string> &v) const {
    impl_->headerNames(v);
}

void
DefaultRequestResponse::addInputHeader(const std::string &name, const std::string &value) {
    impl_->addInputHeader(name, value);
}

unsigned int
DefaultRequestResponse::countCookies() const {
    return impl_->countCookies();
}

bool
DefaultRequestResponse::hasCookie(const std::string &name) const {
    return impl_->hasCookie(name);
}

const std::string&
DefaultRequestResponse::getCookie(const std::string &name) const {
    return impl_->getCookie(name);
}

void
DefaultRequestResponse::cookieNames(std::vector<std::string> &v) const {
    impl_->cookieNames(v);
}

void
DefaultRequestResponse::addInputCookie(const std::string &name, const std::string &value) {
    impl_->addInputCookie(name, value);
}

unsigned int
DefaultRequestResponse::countVariables() const {
    return impl_->countVariables();
}

bool
DefaultRequestResponse::hasVariable(const std::string &name) const {
    return impl_->hasVariable(name);
}

const std::string&
DefaultRequestResponse::getVariable(const std::string &name) const {
    return impl_->getVariable(name);
}

void
DefaultRequestResponse::variableNames(std::vector<std::string> &v) const {
    impl_->variableNames(v);
}

void
DefaultRequestResponse::setVariable(const std::string &name, const std::string &value) {
    impl_->setVariable(name, value);
}

bool
DefaultRequestResponse::hasFile(const std::string &name) const {
    return impl_->hasFile(name);
}

const std::string&
DefaultRequestResponse::remoteFileName(const std::string &name) const {
    return impl_->remoteFileName(name);
}

const std::string&
DefaultRequestResponse::remoteFileType(const std::string &name) const {
    return impl_->remoteFileType(name);
}

std::pair<const char*, std::streamsize>
DefaultRequestResponse::remoteFile(const std::string &name) const {
    return impl_->remoteFile(name);
}

bool
DefaultRequestResponse::isSecure() const {
    return impl_->isSecure();
}

bool
DefaultRequestResponse::isBot() const {
    return impl_->isBot();
}

std::pair<const char*, std::streamsize>
DefaultRequestResponse::requestBody() const {
    return impl_->requestBody();
}

bool
DefaultRequestResponse::suppressBody() const {
    return impl_->suppressBody() || 204 == status_ || 304 == status_;
}

void
DefaultRequestResponse::setCookie(const Cookie &cookie) {
    boost::mutex::scoped_lock sl(resp_mutex_);
    if (!headers_sent_) {
        out_cookies_.insert(cookie);
    }
    else {
        throw std::runtime_error("headers already sent");
    }
}

void
DefaultRequestResponse::setStatus(unsigned short status) {
    boost::mutex::scoped_lock sl(resp_mutex_);
    if (!headers_sent_) {
        status_ = status;
    }
    else {
        throw std::runtime_error("headers already sent");
    }
}

void
DefaultRequestResponse::sendError(unsigned short status, const std::string &message) {
    log()->debug("%s, clearing request output", BOOST_CURRENT_FUNCTION);
    {
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
    }

    boost::mutex::scoped_lock sl(write_mutex_);
    if (detached_) {
        return;
    }

    if (stream_locked_ || isBinaryInternal()) {
        throw std::runtime_error("Cannot write data. Output stream is already occupied");
    }
    sendHeadersInternal();
    writeError(status, message);
}

void
DefaultRequestResponse::setHeader(const std::string &name, const std::string &value) {
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
DefaultRequestResponse::write(const char *buf, std::streamsize size) {
    boost::mutex::scoped_lock wl(write_mutex_);
    if (isBinaryInternal()) {
        throw std::runtime_error("Cannot write data. Output stream is already occupied");
    }

    stream_locked_ = true;
    sendHeadersInternal();
    if (!suppressBody()) {
        if (detached_) {
            return 0;
        }
        writeBuffer(buf, size);
    }
    return size;
}

std::streamsize
DefaultRequestResponse::write(std::auto_ptr<BinaryWriter> writer) {
    boost::mutex::scoped_lock wl(write_mutex_);
    if (detached_) {
        return 0;
    }

    if (stream_locked_ || isBinaryInternal()) {
        throw std::runtime_error("Cannot write data. Output stream is already occupied");
    }

    writer_ = writer;
    return writer_->size();
}

void
DefaultRequestResponse::writeBuffer(const char *buf, std::streamsize size) {
    (void)buf;
    (void)size;
}

void
DefaultRequestResponse::writeError(unsigned short status, const std::string &message) {
    (void)status;
    (void)message;
}

void
DefaultRequestResponse::writeHeaders() {
}

void
DefaultRequestResponse::writeByWriter(const BinaryWriter *writer) {
    (void)writer;
}

std::string
DefaultRequestResponse::outputHeader(const std::string &name) const {
    boost::mutex::scoped_lock sl(resp_mutex_);
    return Parser::get(out_headers_, name);
}

void
DefaultRequestResponse::sendHeaders() {
    boost::mutex::scoped_lock sl(write_mutex_);
    sendHeadersInternal();
}

void
DefaultRequestResponse::sendHeadersInternal() {
    boost::mutex::scoped_lock sl(resp_mutex_);
    std::stringstream stream;
    if (!headers_sent_) {
        log()->debug("%s, sending headers", BOOST_CURRENT_FUNCTION);
        stream << status_ << " " << Parser::statusToString(status_);
        out_headers_["Status"] = stream.str();
        writeHeaders();
        headers_sent_ = true;
    }
}

void
DefaultRequestResponse::attach(std::istream *is, char *env[]) {
    impl_->attach(is, env);
}

void
DefaultRequestResponse::detach() {
    boost::mutex::scoped_lock wl(write_mutex_);
    if (!stream_locked_ && isBinaryInternal()) {   
        setHeader("Content-length", boost::lexical_cast<std::string>(writer_->size()));
        sendHeadersInternal();
        writeByWriter(writer_.get());
    }
    else {
        sendHeadersInternal();
    }
    detached_ = true;
}

const HeaderMap&
DefaultRequestResponse::outHeaders() const {
    return out_headers_;
}
const DefaultRequestResponse::CookieSet&
DefaultRequestResponse::outCookies() const {
    return out_cookies_;
}

bool
DefaultRequestResponse::isBinary() const {
    boost::mutex::scoped_lock wl(write_mutex_);
    return isBinaryInternal();
}

bool
DefaultRequestResponse::isBinaryInternal() const {
    return NULL != writer_.get();
}

} // namespace xscript
