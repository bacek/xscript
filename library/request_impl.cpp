#include "settings.h"

#include <cctype>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <boost/current_function.hpp>

#include "xscript/authorizer.h"
#include "xscript/encoder.h"
#include "xscript/logger.h"
#include "internal/parser.h"
#include "xscript/range.h"
#include "xscript/request_impl.h"
#include "xscript/vhost_data.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

const std::string RequestImpl::HEAD("HEAD");
const std::string RequestImpl::HOST_KEY("HOST");
const std::string RequestImpl::CONTENT_TYPE_KEY("Content-Type");
const std::string RequestImpl::CONTENT_ENCODING_KEY("Content-Encoding");
const std::string RequestImpl::CONTENT_LENGTH_KEY("Content-Length");

const std::string RequestImpl::HTTPS_KEY("HTTPS");
const std::string RequestImpl::SERVER_ADDR_KEY("SERVER_ADDR");
const std::string RequestImpl::SERVER_PORT_KEY("SERVER_PORT");

const std::string RequestImpl::PATH_INFO_KEY("PATH_INFO");
const std::string RequestImpl::PATH_TRANSLATED_KEY("PATH_TRANSLATED");

const std::string RequestImpl::SCRIPT_NAME_KEY("SCRIPT_NAME");
const std::string RequestImpl::SCRIPT_FILENAME_KEY("SCRIPT_FILENAME");
const std::string RequestImpl::DOCUMENT_ROOT_KEY("DOCUMENT_ROOT");

const std::string RequestImpl::REMOTE_USER_KEY("REMOTE_USER");
const std::string RequestImpl::REMOTE_ADDR_KEY("REMOTE_ADDR");

const std::string RequestImpl::QUERY_STRING_KEY("QUERY_STRING");
const std::string RequestImpl::REQUEST_METHOD_KEY("REQUEST_METHOD");

File::File(const std::map<Range, Range, RangeCILess> &m, const Range &content) :
        data_(content.begin(), static_cast<std::streamsize>(content.size())) {
    std::map<Range, Range, RangeCILess>::const_iterator i;
    i = m.find(Parser::CONTENT_TYPE_MULTIPART_RANGE);
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

RequestImpl::RequestImpl() : is_bot_(false) {
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


const std::string&
RequestImpl::getHost() const {
    return getHeader(HOST_KEY);
}

const std::string&
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
    return !val.empty() && strncasecmp(val.c_str(), "on", sizeof("on")) == 0;
}

bool
RequestImpl::isBot() const {
    return is_bot_;
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

    if (is && "POST" == getRequestMethod()) {
        std::streamsize body_length = getContentLength();
        std::streamsize max_body_length = VirtualHostData::instance()->getServer()->maxBodyLength(this);
        if (body_length > max_body_length) {
            std::stringstream stream;
            stream << "Too large body. Length: " << body_length << ". Max allowed: " << max_body_length;
            throw std::runtime_error(stream.str());
        }
        
        body_.resize(getContentLength());
        is->exceptions(std::ios::badbit);
        is->read(&body_[0], body_.size());
        
        if (is->gcount() != static_cast<std::streamsize>(body_.size())) {
            throw std::runtime_error("failed to read request entity");
        }
        
        const std::string &type = getContentType();
        if (strncasecmp("multipart/form-data", type.c_str(), sizeof("multipart/form-data") - 1) == 0) {
            Range body = createRange(body_);
            std::string boundary = Parser::getBoundary(createRange(type));
            Parser::parseMultipart(this, body, boundary, enc.get());
        }
        else if (!body_.empty()) {
            StringUtils::parse(createRange(body_), args_, enc.get());
        }
    }
    else {
        const std::string &query = getQueryString();
        if (!query.empty()) {
            StringUtils::parse(createRange(query), args_, enc.get());
        }
    }

    is_bot_ = Authorizer::instance()->isBot(getHeader("User-Agent"));
}

bool
RequestImpl::normalizeHeader(const std::string &name, const Range &value, std::string &result) {
    (void)name;
    (void)value;
    (void)result;
    return false;
}

std::string
RequestImpl::checkUrlEscaping(const Range &range) {
    std::string result;
    unsigned int length = range.size();
    const char *value = range.begin();
    for(unsigned int i = 0; i < length; ++i, ++value) {
        if (static_cast<unsigned char>(*value) > 127) {
            result.append(StringUtils::urlencode(Range(value, value + 1)));
        }
        else {
            result.push_back(*value);
        }
    }
    return result;
}

Range
RequestImpl::checkHost(const Range &range) {
    Range host(range);
    int length = range.size();
    const char *end_pos = range.begin() + length - 1;
    bool remove_port = false;
    for (int i = 0; i < length; ++i) {
        if (*(end_pos - i) == ':' && i + 1 != length) {
            if (i == 2 && *(end_pos - 1) == '8' && *end_pos == '0') {
                remove_port = true;
            }
            host = Range(range.begin(), end_pos - i);
            break;
        }
    }
     
    for(const char *it = host.begin(); it != host.end(); ++it) {
        if (*it == '/' || *it == ':') {
            throw std::runtime_error("Incorrect host");
        }
    }
    
    if (remove_port) {
        return host;
    }
    
    return range;
}

RequestFactory::RequestFactory() {
}

RequestFactory::~RequestFactory() {
}

std::auto_ptr<RequestImpl>
RequestFactory::create() {
    return std::auto_ptr<RequestImpl>(new RequestImpl());
}

static ComponentRegisterer<RequestFactory> reg;

} // namespace xscript
