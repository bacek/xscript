#include "settings.h"

#include <iostream>
#include <set>

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>

#include "request.h"
#include "xscript/logger.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

StandaloneRequest::StandaloneRequest() :
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
	return args_.size();
}

bool
StandaloneRequest::hasArg(const std::string &name) const {
	boost::mutex::scoped_lock sl(mutex_);
	for (std::vector<StringUtils::NamedValue>::const_iterator i = args_.begin(), end = args_.end(); i != end; ++i) {
		if (i->first == name) {
			return true;
		}
	}
	return false;
}

const std::string&
StandaloneRequest::getArg(const std::string &name) const {
	boost::mutex::scoped_lock sl(mutex_);
	for (std::vector<StringUtils::NamedValue>::const_iterator i = args_.begin(), end = args_.end(); i != end; ++i) {
		if (i->first == name) {
			return i->second;
		}
	}
	return StringUtils::EMPTY_STRING;
}

void
StandaloneRequest::getArg(const std::string &name, std::vector<std::string> &v) const {
	boost::mutex::scoped_lock sl(mutex_);
	std::vector<std::string> tmp;
	tmp.reserve(args_.size());
	for (std::vector<StringUtils::NamedValue>::const_iterator i = args_.begin(), end = args_.end(); i != end; ++i) {
		if (i->first == name) {
			tmp.push_back(i->second);
		}
	}
	v.swap(tmp);
}

void
StandaloneRequest::argNames(std::vector<std::string> &v) const {
	boost::mutex::scoped_lock sl(mutex_);
	std::set<std::string> names;
	for (std::vector<StringUtils::NamedValue>::const_iterator i = args_.begin(), end = args_.end(); i != end; ++i) {
		names.insert(i->first);
	}
	std::vector<std::string> tmp;
	tmp.reserve(names.size());
	std::copy(names.begin(), names.end(), std::back_inserter(tmp));
	v.swap(tmp);
}

void
StandaloneRequest::setArg(const std::string &name, const std::string &value) {
	boost::mutex::scoped_lock sl(mutex_);
	args_.push_back(std::make_pair(name, value));
}

unsigned int
StandaloneRequest::countHeaders() const {
	boost::mutex::scoped_lock lock(mutex_);
	return headers_.size();
}

bool
StandaloneRequest::hasHeader(const std::string &name) const {
	boost::mutex::scoped_lock lock(mutex_);
	return headers_.find(name) != headers_.end();
}

const std::string&
StandaloneRequest::getHeader(const std::string &name) const {
	boost::mutex::scoped_lock lock(mutex_);
	std::map<std::string, std::string>::const_iterator i = headers_.find(name);
	return headers_.end() != i ? i->second : StringUtils::EMPTY_STRING;
}

void
StandaloneRequest::headerNames(std::vector<std::string> &v) const {
	boost::mutex::scoped_lock lock(mutex_);
	std::vector<std::string> tmp;
	tmp.reserve(headers_.size());
	for (std::map<std::string, std::string>::const_iterator i = headers_.begin(), end = headers_.end(); i != end; ++i) {
		tmp.push_back(i->first);
	}
	v.swap(tmp);
}

unsigned int
StandaloneRequest::countCookies() const {
	boost::mutex::scoped_lock lock(mutex_);
	return cookies_.size();
}

bool
StandaloneRequest::hasCookie(const std::string &name) const {
	boost::mutex::scoped_lock lock(mutex_);
	return cookies_.find(name) != cookies_.end();
}

const std::string&
StandaloneRequest::getCookie(const std::string &name) const {
	boost::mutex::scoped_lock lock(mutex_);
	std::map<std::string, std::string>::const_iterator i = cookies_.find(name);
	return cookies_.end() != i ? i->second : StringUtils::EMPTY_STRING;
}

void
StandaloneRequest::cookieNames(std::vector<std::string> &v) const {
	boost::mutex::scoped_lock lock(mutex_);
	std::vector<std::string> tmp;
	tmp.reserve(cookies_.size());
	for (std::map<std::string, std::string>::const_iterator i = cookies_.begin(), end = cookies_.end(); i != end; ++i) {
		tmp.push_back(i->first);
	}
	v.swap(tmp);
}

void
StandaloneRequest::addInputCookie(const std::string &name, const std::string &value) {
	boost::mutex::scoped_lock lock(mutex_);
	cookies_.insert(std::make_pair(name, value));
}

unsigned int
StandaloneRequest::countVariables() const {
	boost::mutex::scoped_lock lock(mutex_);
	return vars_.size();
}

bool
StandaloneRequest::hasVariable(const std::string &name) const {
	boost::mutex::scoped_lock lock(mutex_);
	return vars_.find(name) != vars_.end();
}

const std::string&
StandaloneRequest::getVariable(const std::string &name) const {
	boost::mutex::scoped_lock lock(mutex_);
	std::map<std::string, std::string>::const_iterator i = vars_.find(name);
	return vars_.end() != i ? i->second : StringUtils::EMPTY_STRING;
}

void
StandaloneRequest::variableNames(std::vector<std::string> &v) const {
	boost::mutex::scoped_lock lock(mutex_);
	std::vector<std::string> tmp;
	tmp.reserve(vars_.size());
	for (std::map<std::string, std::string>::const_iterator i = vars_.begin(), end = vars_.end(); i != end; ++i) {
		tmp.push_back(i->first);
	}
	v.swap(tmp);
}

void
StandaloneRequest::setVariable(const std::string &name, const std::string &value) {
	boost::mutex::scoped_lock lock(mutex_);
	vars_.insert(std::make_pair(name, value));
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
	return isSecure_;
}

std::pair<const char*, std::streamsize>
StandaloneRequest::requestBody() const {
	return std::pair<const char*, std::streamsize>(NULL, 0);
}

void
StandaloneRequest::reset() {
	args_.clear();
	headers_.clear();
	vars_.clear();
	cookies_.clear();
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
	return StringUtils::EMPTY_STRING;
}

void
StandaloneRequest::addInputHeader(const std::string &name, const std::string &value) {
	boost::mutex::scoped_lock lock(mutex_);
	headers_.insert(std::make_pair(name, value));
}

void
StandaloneRequest::sendError(unsigned short status, const std::string& message) {
	std::cerr << status << std::endl << message << std::endl;
}

void
StandaloneRequest::sendHeaders() {}

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
