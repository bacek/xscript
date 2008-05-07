#include "settings.h"

#include <iostream>
#include <set>

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>

#include "standalone_request.h"
#include "xscript/logger.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

StandaloneRequest::StandaloneRequest() :
	impl_(RequestFactory::instance()->create()),
	isSecure_(false), port_(80), path_("/"), method_("GET")
{
}

StandaloneRequest::~StandaloneRequest() {
}

unsigned short
StandaloneRequest::getServerPort() const {
	return port_;
}

const std::string&
StandaloneRequest::getServerAddr() const {
	return host_;
}

const std::string&
StandaloneRequest::getPathInfo() const {
	return path_;
}

const std::string&
StandaloneRequest::getPathTranslated() const {
	return path_;
}

const std::string&
StandaloneRequest::getScriptName() const {
	return script_name_;
}

const std::string&
StandaloneRequest::getScriptFilename() const {
	return script_file_name_;
}

const std::string&
StandaloneRequest::getDocumentRoot() const {
	return root_directory_;
}

const std::string&
StandaloneRequest::getRemoteUser() const {
	return StringUtils::EMPTY_STRING;
}

const std::string&
StandaloneRequest::getRemoteAddr() const {
	return StringUtils::EMPTY_STRING;
}

const std::string&
StandaloneRequest::getRealIP() const {
	return getRemoteAddr();
}

const std::string&
StandaloneRequest::getQueryString() const {
	return query_;
}

const std::string&
StandaloneRequest::getRequestMethod() const {
	return method_;
}

std::string
StandaloneRequest::getURI() const {
	return impl_->getURI();
}

std::string
StandaloneRequest::getOriginalURI() const {
	return impl_->getOriginalURI();
}

std::string
StandaloneRequest::getHost() const {
	return impl_->getHost();
}

std::string
StandaloneRequest::getOriginalHost() const {
	return impl_->getOriginalHost();
}

std::streamsize
StandaloneRequest::getContentLength() const {
	return 0;
}

const std::string&
StandaloneRequest::getContentType() const {
	return StringUtils::EMPTY_STRING;
}

const std::string&
StandaloneRequest::getContentEncoding() const {
	return StringUtils::EMPTY_STRING;
}

unsigned int
StandaloneRequest::countArgs() const {
	boost::mutex::scoped_lock sl(mutex_);
	return impl_->countArgs();
}

bool
StandaloneRequest::hasArg(const std::string &name) const {
	boost::mutex::scoped_lock sl(mutex_);
	return impl_->hasArg(name);
}

const std::string&
StandaloneRequest::getArg(const std::string &name) const {
	boost::mutex::scoped_lock sl(mutex_);
	return impl_->getArg(name);
}

void
StandaloneRequest::getArg(const std::string &name, std::vector<std::string> &v) const {
	boost::mutex::scoped_lock sl(mutex_);
	return impl_->getArg(name, v);
}

void
StandaloneRequest::argNames(std::vector<std::string> &v) const {
	boost::mutex::scoped_lock sl(mutex_);
	impl_->argNames(v);
}

void
StandaloneRequest::setArg(const std::string &name, const std::string &value) {
	boost::mutex::scoped_lock sl(mutex_);
	impl_->setArg(name, value);
}

unsigned int
StandaloneRequest::countHeaders() const {
	boost::mutex::scoped_lock lock(mutex_);
	return impl_->countHeaders();
}

bool
StandaloneRequest::hasHeader(const std::string &name) const {
	boost::mutex::scoped_lock lock(mutex_);
	return impl_->hasHeader(name);
}

const std::string&
StandaloneRequest::getHeader(const std::string &name) const {
	boost::mutex::scoped_lock lock(mutex_);
	return impl_->getHeader(name);
}

void
StandaloneRequest::headerNames(std::vector<std::string> &v) const {
	boost::mutex::scoped_lock lock(mutex_);
	impl_->headerNames(v);
}


void
StandaloneRequest::addInputHeader(const std::string &name, const std::string &value) {
	boost::mutex::scoped_lock lock(mutex_);
	impl_->addInputHeader(name, value);
}

unsigned int
StandaloneRequest::countCookies() const {
	boost::mutex::scoped_lock lock(mutex_);
	return impl_->countCookies();
}

bool
StandaloneRequest::hasCookie(const std::string &name) const {
	boost::mutex::scoped_lock lock(mutex_);
	return impl_->hasCookie(name);
}

const std::string&
StandaloneRequest::getCookie(const std::string &name) const {
	boost::mutex::scoped_lock lock(mutex_);
	return impl_->getCookie(name);
}

void
StandaloneRequest::cookieNames(std::vector<std::string> &v) const {
	boost::mutex::scoped_lock lock(mutex_);
	impl_->cookieNames(v);
}

void
StandaloneRequest::addInputCookie(const std::string &name, const std::string &value) {
	boost::mutex::scoped_lock lock(mutex_);
	impl_->addInputCookie(name, value);
}

unsigned int
StandaloneRequest::countVariables() const {
	boost::mutex::scoped_lock lock(mutex_);
	return impl_->countVariables();
}

bool
StandaloneRequest::hasVariable(const std::string &name) const {
	boost::mutex::scoped_lock lock(mutex_);
	return impl_->hasVariable(name);
}

const std::string&
StandaloneRequest::getVariable(const std::string &name) const {
	boost::mutex::scoped_lock lock(mutex_);
	return impl_->getVariable(name);
}

void
StandaloneRequest::variableNames(std::vector<std::string> &v) const {
	boost::mutex::scoped_lock lock(mutex_);
	impl_->variableNames(v);
}

void
StandaloneRequest::setVariable(const std::string &name, const std::string &value) {
	boost::mutex::scoped_lock lock(mutex_);
	impl_->setVariable(name, value);
}

bool
StandaloneRequest::hasFile(const std::string &name) const {
	return impl_->hasFile(name);
}

const std::string&
StandaloneRequest::remoteFileName(const std::string &name) const {
	return impl_->remoteFileName(name);
}

const std::string&
StandaloneRequest::remoteFileType(const std::string &name) const {
	return impl_->remoteFileType(name);
}

std::pair<const char*, std::streamsize>
StandaloneRequest::remoteFile(const std::string &name) const {
	return impl_->remoteFile(name);
}

bool
StandaloneRequest::isSecure() const {
	return isSecure_;
}

std::pair<const char*, std::streamsize>
StandaloneRequest::requestBody() const {
	return std::pair<const char*, std::streamsize>(NULL, 0);
}

void
StandaloneRequest::reset() {
	impl_->reset();
}

void
StandaloneRequest::setCookie(const Cookie &cookie) {
	(void)cookie;
}

void
StandaloneRequest::setStatus(unsigned short status) {
	(void)status;
}

void
StandaloneRequest::setHeader(const std::string &name, const std::string &value) {
	(void)name;
	(void)value;
}

std::streamsize
StandaloneRequest::write(const char *buf, std::streamsize size) {
	std::cout.write(buf, size);
	return size;
}

std::string
StandaloneRequest::outputHeader(const std::string &name) const {
	(void)name;
	return StringUtils::EMPTY_STRING;
}

void
StandaloneRequest::sendError(unsigned short status, const std::string& message) {
	std::cerr << status << std::endl << message << std::endl;
}

void
StandaloneRequest::sendHeaders() {
}

bool
StandaloneRequest::suppressBody() const {
	return false;
}

void
StandaloneRequest::attach(const std::string& url, const std::string& doc_root) {
	root_directory_ = doc_root;

	boost::regex expression(
		"^(\?:([^:/\?#]+)://)\?(\\w+[^/\?#:]*)\?(\?::(\\d+))\?(/\?(\?:[^\?#/]*/)*)\?([^\?#]*)\?(\\\?.*)\?");
	boost::cmatch matches;
	if (boost::regex_match(url.c_str(), matches, expression)) {
		std::string match(matches[1].first, matches[1].second);
		isSecure_ = false;
		if ("https" == std::string(matches[1].first, matches[1].second)) {
			isSecure_ = true;
		}

		host_ = std::string(matches[2].first, matches[2].second);
		match = std::string(matches[3].first, matches[3].second);
		port_ = 80;
		if (!match.empty()) {
			port_ = boost::lexical_cast<unsigned short>(match);
		}

		path_ = std::string(matches[4].first, matches[4].second);
		if (path_.empty()) {
			path_ = "/";
		}

		script_name_ = std::string(matches[5].first, matches[5].second);
		if (script_name_.empty()) {
			script_name_.swap(host_);
		}

		script_file_name_ = root_directory_ + path_ + script_name_;
		query_ = std::string(matches[6].first, matches[6].second).erase(0, 1);
	}
	else {
		throw std::runtime_error("Cannot parse url: " + url);
	}

	typedef boost::char_separator<char> Separator;
	typedef boost::tokenizer<Separator> Tokenizer;

	Tokenizer tok(query_, Separator("&"));
	for(Tokenizer::iterator it = tok.begin(), it_end = tok.end(); it != it_end; ++it) {
		std::string::size_type pos = it->find('=');
		if (std::string::npos != pos && pos < it->size() - 1) {
			setArg(it->substr(0, pos), it->substr(pos + 1));
		}
	}
}

} // namespace xscript
