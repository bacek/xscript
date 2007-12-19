#include "settings.h"

#include <iostream>

#include "request.h"
#include "xscript/logger.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

StandaloneRequest::StandaloneRequest(const std::string &url)
{
}

StandaloneRequest::~StandaloneRequest() {
}

unsigned short
StandaloneRequest::getServerPort() const {
	return 0;
}

const std::string&
StandaloneRequest::getServerAddr() const {

}

const std::string&
StandaloneRequest::getPathInfo() const {
}

const std::string&
StandaloneRequest::getPathTranslated() const {
}

const std::string&
StandaloneRequest::getScriptName() const {
}

const std::string&
StandaloneRequest::getScriptFilename() const {
}

const std::string&
StandaloneRequest::getDocumentRoot() const {
}

const std::string&
StandaloneRequest::getRemoteUser() const {
}

const std::string&
StandaloneRequest::getRemoteAddr() const {
}

const std::string&
StandaloneRequest::getQueryString() const {
	
}

const std::string&
StandaloneRequest::getRequestMethod() const {
	return "GET";
}

std::streamsize
StandaloneRequest::getContentLength() const {
	
}

const std::string&
StandaloneRequest::getContentType() const {
}
	
unsigned int
StandaloneRequest::countArgs() const {
	return params_.size();
}

bool
StandaloneRequest::hasArg(const std::string &name) const {
}

const std::string&
StandaloneRequest::getArg(const std::string &name) const {
}

void
StandaloneRequest::getArg(const std::string &name, std::vector<std::string> &v) const {
}

void
StandaloneRequest::argNames(std::vector<std::string> &v) const {
}

unsigned int
StandaloneRequest::countHeaders() const {
	return headers_.size();
}

bool
StandaloneRequest::hasHeader(const std::string &name) const {
	return headers_.find(name) != headers_.end();
}

const std::string&
StandaloneRequest::getHeader(const std::string &name) const {
	std::map<std::string, std::string>::const_iterator i = headers_.find(name);
	return headers_.end() != i ? i->second : StringUtils::EMPTY_STRING;
}

void
StandaloneRequest::headerNames(std::vector<std::string> &v) const {
	std::vector<std::string> tmp;
	tmp.reserve(headers_.size());
	for (std::map<std::string, std::string>::const_iterator i = headers_.begin(), end = headers_.end(); i != end; ++i) {
		tmp.push_back(i->first);
	}
	v.swap(tmp);
}

unsigned int
StandaloneRequest::countCookies() const {
	return 0;
}

bool
StandaloneRequest::hasCookie(const std::string &name) const {
	return false;
}

const std::string&
StandaloneRequest::getCookie(const std::string &name) const {
	return StringUtils::EMPTY_STRING;
}

void
StandaloneRequest::cookieNames(std::vector<std::string> &v) const {
	v.clear();
}

unsigned int
StandaloneRequest::countVariables() const {
	return 0;
}

bool
StandaloneRequest::hasVariable(const std::string &name) const {
	return false;
}

const std::string&
StandaloneRequest::getVariable(const std::string &name) const {
	return StringUtils::EMPTY_STRING;
}

void
StandaloneRequest::variableNames(std::vector<std::string> &v) const {
	v.clear();
}

bool
StandaloneRequest::hasFile(const std::string &name) const {
	return false;
}

const std::string&
StandaloneRequest::remoteFileName(const std::string &name) const {
	return StringUtils::EMPTY_STRING;
}

const std::string&
StandaloneRequest::remoteFileType(const std::string &name) const {
	return StringUtils::EMPTY_STRING;
}

std::pair<const char*, std::streamsize>
StandaloneRequest::remoteFile(const std::string &name) const {
	return std::pair<const char*, std::streamsize>(NULL, 0);
}

bool
StandaloneRequest::isSecure() const {
}

std::pair<const char*, std::streamsize>
StandaloneRequest::requestBody() const {
	return std::pair<const char*, std::streamsize>(NULL, 0);
}

void
StandaloneRequest::clear() {
	headers_.clear();
}

void
StandaloneRequest::setCookie(const Cookie &cookie) {
}

void
StandaloneRequest::setStatus(unsigned short status) {
}

void
StandaloneRequest::setHeader(const std::string &name, const std::string &value) {
}

std::streamsize
StandaloneRequest::write(const char *buf, std::streamsize size) {
	std::cout.write(buf, size);
	return size;
}

std::string
StandaloneRequest::outputHeader(const std::string &name) const {
	return std::string();
}

void
StandaloneRequest::addHeader(const std::pair<std::string, std::string> &header) {
	headers_.insert(header);
}

} // namespace xscript
