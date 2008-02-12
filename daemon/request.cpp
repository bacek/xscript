#include "settings.h"

#include <cctype>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/current_function.hpp>

#include "parser.h"
#include "request.h"
#include "xscript/range.h"
#include "xscript/logger.h"
#include "xscript/encoder.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

static const std::string HEAD("HEAD");
static const std::string HOST_KEY("HOST");
static const std::string CONTENT_TYPE_KEY("Content-Type");
static const std::string CONTENT_LENGTH_KEY("Content-Length");

static const std::string HTTPS_KEY("HTTPS");
static const std::string SERVER_ADDR_KEY("SERVER_ADDR");
static const std::string SERVER_PORT_KEY("SERVER_PORT");

static const std::string PATH_INFO_KEY("PATH_INFO");
static const std::string PATH_TRANSLATED_KEY("PATH_TRANSLATED");

static const std::string SCRIPT_NAME_KEY("SCRIPT_NAME");
static const std::string SCRIPT_FILENAME_KEY("SCRIPT_FILENAME");
static const std::string DOCUMENT_ROOT_KEY("DOCUMENT_ROOT");

static const std::string REMOTE_USER_KEY("REMOTE_USER");
static const std::string REMOTE_ADDR_KEY("REMOTE_ADDR");

static const std::string QUERY_STRING_KEY("QUERY_STRING");
static const std::string REQUEST_METHOD_KEY("REQUEST_METHOD");

File::File(const std::map<Range, Range, RangeCILess> &m, const Range &content) :
	data_(content.begin(), static_cast<std::streamsize>(content.size()))
{
	std::map<Range, Range, RangeCILess>::const_iterator i;
	i = m.find(Parser::CONTENT_TYPE_RANGE);
	if (m.end() != i) {
		type_.assign(i->second.begin(), i->second.end());
	}
	i = m.find(Parser::FILENAME_RANGE);
	if (m.end() != i) {
		name_.assign(i->second.begin(), i->second.end());
	}
	else {
		throw std::runtime_error("uploaded file without name"); 
	}
}

const std::string&
File::type() const {
	return type_;
}

const std::string&
File::remoteName() const {
	return name_;
}

std::pair<const char*, std::streamsize>
File::data() const {
	return data_;
}

ServerRequest::ServerRequest() {
	reset();
}

ServerRequest::~ServerRequest() {
}

unsigned short
ServerRequest::getServerPort() const {
	const std::string &res = Parser::get(vars_, SERVER_PORT_KEY);
	return (!res.empty()) ? boost::lexical_cast<unsigned short>(res) : 80;
}

const std::string&
ServerRequest::getServerAddr() const {
	return Parser::get(vars_, SERVER_ADDR_KEY);
}

const std::string&
ServerRequest::getPathInfo() const {
	return Parser::get(vars_, PATH_INFO_KEY);
}

const std::string&
ServerRequest::getPathTranslated() const {
		return Parser::get(vars_, PATH_TRANSLATED_KEY);
}

const std::string&
ServerRequest::getScriptName() const {
	return Parser::get(vars_, SCRIPT_NAME_KEY);
}

const std::string&
ServerRequest::getScriptFilename() const {
	return Parser::get(vars_, SCRIPT_FILENAME_KEY);
}

const std::string&
ServerRequest::getDocumentRoot() const {
	return Parser::get(vars_, DOCUMENT_ROOT_KEY);
}

const std::string&
ServerRequest::getRemoteUser() const {
	return Parser::get(vars_, REMOTE_USER_KEY);
}

const std::string&
ServerRequest::getRemoteAddr() const {
	return Parser::get(vars_, REMOTE_ADDR_KEY);
}

const std::string&
ServerRequest::getQueryString() const {
	return Parser::get(vars_, QUERY_STRING_KEY);
}

const std::string&
ServerRequest::getRequestMethod() const {
	return Parser::get(vars_, REQUEST_METHOD_KEY);
}

std::streamsize
ServerRequest::getContentLength() const {
	return boost::lexical_cast<std::streamsize>(
		Parser::get(headers_, CONTENT_LENGTH_KEY));
}

const std::string&
ServerRequest::getContentType() const {
	return Parser::get(headers_, CONTENT_TYPE_KEY);
}

unsigned int
ServerRequest::countArgs() const {
	return args_.size();
}

bool
ServerRequest::hasArg(const std::string &name) const {
	for (std::vector<StringUtils::NamedValue>::const_iterator i = args_.begin(), end = args_.end(); i != end; ++i) {
		if (i->first == name) {
			return true;
		}
	}
	return false;
}

const std::string&
ServerRequest::getArg(const std::string &name) const {
	for (std::vector<StringUtils::NamedValue>::const_iterator i = args_.begin(), end = args_.end(); i != end; ++i) {
		if (i->first == name) {
			return i->second;
		}
	}
	return StringUtils::EMPTY_STRING;
}

void
ServerRequest::getArg(const std::string &name, std::vector<std::string> &v) const {
	
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
ServerRequest::argNames(std::vector<std::string> &v) const {
	std::set<std::string> names;
	for (std::vector<StringUtils::NamedValue>::const_iterator i = args_.begin(), end = args_.end(); i != end; ++i) {
		names.insert(i->first);
	}
	std::vector<std::string> tmp;
	tmp.reserve(names.size());
	std::copy(names.begin(), names.end(), std::back_inserter(tmp));
	v.swap(tmp);
}

unsigned int
ServerRequest::countHeaders() const {
	return headers_.size();
}

bool
ServerRequest::hasHeader(const std::string &name) const {
	return Parser::has(headers_, name);
}

const std::string&
ServerRequest::getHeader(const std::string &name) const {
	return Parser::get(headers_, name);
}

void
ServerRequest::headerNames(std::vector<std::string> &v) const {
	Parser::keys(headers_, v);
}

unsigned int
ServerRequest::countCookies() const {
	return cookies_.size();
}

bool
ServerRequest::hasCookie(const std::string &name) const {
	return Parser::has(cookies_, name);
}

const std::string&
ServerRequest::getCookie(const std::string &name) const {
	return Parser::get(cookies_, name);
}

void
ServerRequest::cookieNames(std::vector<std::string> &v) const {
	Parser::keys(cookies_, v);
}

unsigned int
ServerRequest::countVariables() const {
	return vars_.size();
}

bool
ServerRequest::hasVariable(const std::string &name) const {
	return Parser::has(vars_, name);
}

const std::string&
ServerRequest::getVariable(const std::string &name) const {
	return Parser::get(vars_, name);
}

void
ServerRequest::variableNames(std::vector<std::string> &v) const {
	Parser::keys(vars_, v);
}

bool
ServerRequest::hasFile(const std::string &name) const {
	return files_.find(name) != files_.end();
}

const std::string&
ServerRequest::remoteFileName(const std::string &name) const {
	std::map<std::string, File>::const_iterator i = files_.find(name);
	if (files_.end() != i) {
		return i->second.remoteName();
	}
	return StringUtils::EMPTY_STRING;
}

const std::string&
ServerRequest::remoteFileType(const std::string &name) const {
	std::map<std::string, File>::const_iterator i = files_.find(name);
	if (files_.end() != i) {
		return i->second.type();
	}
	return StringUtils::EMPTY_STRING;
}

std::pair<const char*, std::streamsize>
ServerRequest::remoteFile(const std::string &name) const {
	std::map<std::string, File>::const_iterator i = files_.find(name);
	if (files_.end() != i) {
		return i->second.data();
	}
	return std::make_pair<const char*, std::streamsize>(NULL, 0);
}

bool
ServerRequest::isSecure() const {
	const std::string &val = Parser::get(vars_, HTTPS_KEY);
	return !val.empty() && ("on" == val);
}

std::pair<const char*, std::streamsize>
ServerRequest::requestBody() const {
	return std::make_pair<const char*, std::streamsize>(&body_[0], body_.size());
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
	return HEAD == getRequestMethod() || 204 == status_ || 304 == status_;
}

void
ServerRequest::reset() {
	
	status_ = 200;
	stream_ = NULL;
	headers_sent_ = false;

	body_.clear();
	args_.clear();
	vars_.clear();
	
	files_.clear();
	cookies_.clear();
	headers_.clear();
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
	
	std::auto_ptr<Encoder> enc = Encoder::createDefault("cp1251", "UTF-8");
	Parser::parse(this, env, enc.get());
	
	if ("POST" == getRequestMethod()) {
		
		body_.resize(getContentLength());
		is->exceptions(std::ios::badbit);
		is->read(&body_[0], body_.size());
		
		if (is->gcount() != static_cast<std::streamsize>(body_.size())) {
			throw std::runtime_error("failed to read request entity");
		}
	}
	else {
		const std::string &query = getQueryString();
		body_.assign(query.begin(), query.end());
	}

	const std::string &type = getContentType();
	if (strncasecmp("multipart/form-data", type.c_str(), sizeof("multipart/form-data") - 1) == 0) {
		Range body = createRange(body_);
		std::string boundary = Parser::getBoundary(createRange(type));
		Parser::parseMultipart(this, body, boundary, enc.get());
	}
	else {
		StringUtils::parse(createRange(body_), args_, enc.get());
	}
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
			(*stream_) << "set-Cookie: " << i->toString() << "\r\n";
		}
		(*stream_) << "\r\n";
		headers_sent_ = true;
	}
}

void
ServerRequest::addInputHeader(const std::string &name, const std::string &value) {

	Range key_range = createRange(name);
	Range value_range = createRange(value);
	std::auto_ptr<Encoder> enc = Encoder::createDefault("cp1251", "UTF-8");
	Parser::addHeader(this, key_range, value_range, enc.get());
}

} // namespace xscript
