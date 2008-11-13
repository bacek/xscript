#include "settings.h"

#include <cctype>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <boost/current_function.hpp>

#include "xscript/request_impl.h"
#include "internal/parser.h"
#include "xscript/range.h"
#include "xscript/logger.h"
#include "xscript/encoder.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

static const std::string HEAD("HEAD");
static const std::string HOST_KEY("HOST");
static const std::string CONTENT_TYPE_KEY("Content-Type");
static const std::string CONTENT_ENCODING_KEY("Content-Encoding");
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
        data_(content.begin(), static_cast<std::streamsize>(content.size())) {
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

RequestImpl::RequestImpl() {
}

RequestImpl::~RequestImpl() {
}

unsigned short
RequestImpl::getServerPort() const {
    const std::string &res = Parser::get(vars_, SERVER_PORT_KEY);
    return (!res.empty()) ? boost::lexical_cast<unsigned short>(res) : 80;
}

const std::string&
RequestImpl::getServerAddr() const {
    return Parser::get(vars_, SERVER_ADDR_KEY);
}

const std::string&
RequestImpl::getPathInfo() const {
    return Parser::get(vars_, PATH_INFO_KEY);
}

const std::string&
RequestImpl::getPathTranslated() const {
    return Parser::get(vars_, PATH_TRANSLATED_KEY);
}

const std::string&
RequestImpl::getScriptName() const {
    return Parser::get(vars_, SCRIPT_NAME_KEY);
}

const std::string&
RequestImpl::getScriptFilename() const {
    return Parser::get(vars_, SCRIPT_FILENAME_KEY);
}

const std::string&
RequestImpl::getDocumentRoot() const {
    return Parser::get(vars_, DOCUMENT_ROOT_KEY);
}

const std::string&
RequestImpl::getRemoteUser() const {
    return Parser::get(vars_, REMOTE_USER_KEY);
}

const std::string&
RequestImpl::getRemoteAddr() const {
    return Parser::get(vars_, REMOTE_ADDR_KEY);
}

const std::string&
RequestImpl::getRealIP() const {
    return getRemoteAddr();
}

const std::string&
RequestImpl::getQueryString() const {
    return Parser::get(vars_, QUERY_STRING_KEY);
}

const std::string&
RequestImpl::getRequestMethod() const {
    return Parser::get(vars_, REQUEST_METHOD_KEY);
}

std::string
RequestImpl::getURI() const {
    const std::string& script_name = getScriptName();
    const std::string& query_string = getQueryString();
    if (query_string.empty()) {
        return script_name + getPathInfo();
    }
    else {
        return script_name + getPathInfo() + "?" + query_string;
    }
}

std::string
RequestImpl::getOriginalURI() const {
    return getURI();
}

std::string
RequestImpl::getOriginalUrl() const {
    std::string url(isSecure() ? "https://" : "http://");
    url.append(getOriginalHost());
    url.append(getOriginalURI());
    return url;
}


std::string
RequestImpl::getHost() const {
    const std::string& host_header = getHeader(HOST_KEY);
    if (host_header.empty()) {
        return StringUtils::EMPTY_STRING;
    }

    if (!isSecure() && host_header.find(':') == std::string::npos) {
        int port = getServerPort();
        if (port != 80) {
            return std::string(host_header).append(":").append(boost::lexical_cast<std::string>(port));
        }
    }

    return host_header;
}

std::string
RequestImpl::getOriginalHost() const {
    return getHost();
}

std::streamsize
RequestImpl::getContentLength() const {
    const std::string& length = getHeader(CONTENT_LENGTH_KEY);
    if (length.empty()) {
        return 0;
    }
    try {
        return boost::lexical_cast<std::streamsize>(length);
    }
    catch (const boost::bad_lexical_cast &e) {
        return 0;
    }
}

const std::string&
RequestImpl::getContentType() const {
    return getHeader(CONTENT_TYPE_KEY);
}

const std::string&
RequestImpl::getContentEncoding() const {
    return getHeader(CONTENT_ENCODING_KEY);
}

unsigned int
RequestImpl::countArgs() const {
    return args_.size();
}

bool
RequestImpl::hasArg(const std::string &name) const {
    for (std::vector<StringUtils::NamedValue>::const_iterator i = args_.begin(), end = args_.end(); i != end; ++i) {
        if (i->first == name) {
            return true;
        }
    }
    return false;
}

const std::string&
RequestImpl::getArg(const std::string &name) const {
    for (std::vector<StringUtils::NamedValue>::const_iterator i = args_.begin(), end = args_.end(); i != end; ++i) {
        if (i->first == name) {
            return i->second;
        }
    }
    return StringUtils::EMPTY_STRING;
}

void
RequestImpl::getArg(const std::string &name, std::vector<std::string> &v) const {
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
RequestImpl::setArg(const std::string &name, const std::string &value) {
    args_.push_back(std::make_pair(name, value));
}

void
RequestImpl::argNames(std::vector<std::string> &v) const {
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
RequestImpl::countHeaders() const {
    boost::mutex::scoped_lock lock(mutex_);
    return headers_.size();
}

bool
RequestImpl::hasHeader(const std::string &name) const {
    boost::mutex::scoped_lock lock(mutex_);
    return Parser::has(headers_, name);
}

const std::string&
RequestImpl::getHeader(const std::string &name) const {
    boost::mutex::scoped_lock lock(mutex_);
    return Parser::get(headers_, name);
}

void
RequestImpl::headerNames(std::vector<std::string> &v) const {
    boost::mutex::scoped_lock lock(mutex_);
    Parser::keys(headers_, v);
}

unsigned int
RequestImpl::countCookies() const {
    return cookies_.size();
}

bool
RequestImpl::hasCookie(const std::string &name) const {
    return Parser::has(cookies_, name);
}

const std::string&
RequestImpl::getCookie(const std::string &name) const {
    return Parser::get(cookies_, name);
}

void
RequestImpl::cookieNames(std::vector<std::string> &v) const {
    Parser::keys(cookies_, v);
}

void
RequestImpl::addInputCookie(const std::string &name, const std::string &value) {
    cookies_.insert(std::make_pair(name, value));
}

unsigned int
RequestImpl::countVariables() const {
    return vars_.size();
}

bool
RequestImpl::hasVariable(const std::string &name) const {
    return Parser::has(vars_, name);
}

const std::string&
RequestImpl::getVariable(const std::string &name) const {
    return Parser::get(vars_, name);
}

void
RequestImpl::variableNames(std::vector<std::string> &v) const {
    Parser::keys(vars_, v);
}

void
RequestImpl::setVariable(const std::string &name, const std::string &value) {
    vars_.insert(std::make_pair(name, value));
}

bool
RequestImpl::hasFile(const std::string &name) const {
    return files_.find(name) != files_.end();
}

const std::string&
RequestImpl::remoteFileName(const std::string &name) const {
    std::map<std::string, File>::const_iterator i = files_.find(name);
    if (files_.end() != i) {
        return i->second.remoteName();
    }
    return StringUtils::EMPTY_STRING;
}

const std::string&
RequestImpl::remoteFileType(const std::string &name) const {
    std::map<std::string, File>::const_iterator i = files_.find(name);
    if (files_.end() != i) {
        return i->second.type();
    }
    return StringUtils::EMPTY_STRING;
}

std::pair<const char*, std::streamsize>
RequestImpl::remoteFile(const std::string &name) const {
    std::map<std::string, File>::const_iterator i = files_.find(name);
    if (files_.end() != i) {
        return i->second.data();
    }
    return std::make_pair<const char*, std::streamsize>(NULL, 0);
}

bool
RequestImpl::isSecure() const {
    const std::string &val = Parser::get(vars_, HTTPS_KEY);
    return !val.empty() && ("on" == val);
}

std::pair<const char*, std::streamsize>
RequestImpl::requestBody() const {
    return std::make_pair<const char*, std::streamsize>(&body_[0], body_.size());
}

bool
RequestImpl::suppressBody() const {
    return HEAD == getRequestMethod();
}

void
RequestImpl::addInputHeader(const std::string &name, const std::string &value) {
    Range key_range = createRange(name);
    Range value_range = createRange(value);
    std::auto_ptr<Encoder> enc = Encoder::createDefault("cp1251", "UTF-8");
    boost::mutex::scoped_lock lock(mutex_);
    Parser::addHeader(this, key_range, value_range, enc.get());
}

void
RequestImpl::attach(std::istream *is, char *env[]) {

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
}


RequestFactory::RequestFactory() {
}

RequestFactory::~RequestFactory() {
}

std::auto_ptr<RequestImpl>
RequestFactory::create() {
    return std::auto_ptr<RequestImpl>(new RequestImpl());
}

REGISTER_COMPONENT(RequestFactory);

} // namespace xscript
