#include "settings.h"

#include <iostream>

#include "internal/default_request_response.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

DefaultRequestResponse::DefaultRequestResponse() :
    impl_(RequestFactory::instance()->create()) {
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
    boost::mutex::scoped_lock lock(mutex_);
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
    boost::mutex::scoped_lock lock(mutex_);
    return impl_->getOriginalURI();
}

std::string
DefaultRequestResponse::getHost() const {
    boost::mutex::scoped_lock lock(mutex_);
    return impl_->getHost();
}

std::string
DefaultRequestResponse::getOriginalHost() const {
    boost::mutex::scoped_lock lock(mutex_);
    return impl_->getOriginalHost();
}

std::string
DefaultRequestResponse::getOriginalUrl() const {
    boost::mutex::scoped_lock lock(mutex_);
    return impl_->getOriginalUrl();
}

std::streamsize
DefaultRequestResponse::getContentLength() const {
    boost::mutex::scoped_lock lock(mutex_);
    return impl_->getContentLength();
}

const std::string&
DefaultRequestResponse::getContentType() const {
    boost::mutex::scoped_lock lock(mutex_);
    return impl_->getContentType();
}

const std::string&
DefaultRequestResponse::getContentEncoding() const {
    boost::mutex::scoped_lock lock(mutex_);
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
    boost::mutex::scoped_lock lock(mutex_);
    return impl_->countHeaders();
}

bool
DefaultRequestResponse::hasHeader(const std::string &name) const {
    boost::mutex::scoped_lock lock(mutex_);
    return impl_->hasHeader(name);
}

const std::string&
DefaultRequestResponse::getHeader(const std::string &name) const {
    boost::mutex::scoped_lock lock(mutex_);
    return impl_->getHeader(name);
}

void
DefaultRequestResponse::headerNames(std::vector<std::string> &v) const {
    boost::mutex::scoped_lock lock(mutex_);
    impl_->headerNames(v);
}

void
DefaultRequestResponse::addInputHeader(const std::string &name, const std::string &value) {
    boost::mutex::scoped_lock lock(mutex_);
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

std::pair<const char*, std::streamsize>
DefaultRequestResponse::requestBody() const {
    return impl_->requestBody();
}

bool
DefaultRequestResponse::suppressBody() const {
    return impl_->suppressBody();
}

void
DefaultRequestResponse::setCookie(const Cookie &cookie) {
    (void)cookie;
}

void
DefaultRequestResponse::setStatus(unsigned short status) {
    (void)status;
}

void
DefaultRequestResponse::sendError(unsigned short status, const std::string& message) {
    (void)status;
    (void)message;
}

void
DefaultRequestResponse::setHeader(const std::string &name, const std::string &value) {
    (void)name;
    (void)value;
}

std::streamsize
DefaultRequestResponse::write(const char *buf, std::streamsize size) {
    (void)buf;
    return size;
}

std::string
DefaultRequestResponse::outputHeader(const std::string &name) const {
    (void)name;
    return StringUtils::EMPTY_STRING;
}

void
DefaultRequestResponse::sendHeaders() {
}

void
DefaultRequestResponse::attach(std::istream *is, char *env[]) {
    impl_->attach(is, env);
}

} // namespace xscript
