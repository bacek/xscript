#include "settings.h"

#include "xscript/algorithm.h"
#include "xscript/authorizer.h"
#include "xscript/encoder.h"
#include "xscript/exception.h"
#include "xscript/logger.h"
#include "xscript/message_interface.h"
#include "xscript/response.h"
#include "xscript/request.h"

#include <boost/lexical_cast.hpp>
#include <boost/thread/mutex.hpp>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

#if defined(HAVE_GNUCXX_HASHMAP)

typedef __gnu_cxx::hash_map<std::string, std::string, StringCIHash> VarMap;

#elif defined(HAVE_EXT_HASH_MAP) || defined(HAVE_STLPORT_HASHMAP)

typedef std::hash_map<std::string, std::string, StringCIHash> VarMap;

#else

typedef std::map<std::string, std::string> VarMap;

#endif

class File {
public:
    File(const std::map<Range, Range, RangeCILess> &m, const Range &content);

    const std::string& type() const;
    const std::string& remoteName() const;

    std::pair<const char*, std::streamsize> data() const;

private:
    std::string name_, type_;
    std::pair<const char*, std::streamsize> data_;
};

const std::string Request::ATTACH_METHOD = "REQUEST_ATTACH";
const std::string Request::REAL_IP_METHOD = "REQUEST_REAL_IP";
const std::string Request::ORIGINAL_URI_METHOD = "REQUEST_ORIGINAL_URI";
const std::string Request::ORIGINAL_HOST_METHOD = "REQUEST_ORIGINAL_HOST";



class Request::Request_Data {
public:
    Request_Data();
    ~Request_Data();
    
    mutable boost::mutex mutex_;
    VarMap vars_, cookies_;
    std::vector<char> body_;
    HeaderMap headers_;
    
    std::map<std::string, File> files_;
    std::vector<StringUtils::NamedValue> args_;
    bool is_bot_;

    static const std::string HEAD;
    static const std::string HOST_KEY;
    static const std::string CONTENT_TYPE_KEY;
    static const std::string CONTENT_ENCODING_KEY;
    static const std::string CONTENT_LENGTH_KEY;
    
    static const std::string HTTPS_KEY;
    static const std::string SERVER_ADDR_KEY;
    static const std::string SERVER_PORT_KEY;
    
    static const std::string PATH_INFO_KEY;
    static const std::string PATH_TRANSLATED_KEY;
    
    static const std::string SCRIPT_NAME_KEY;
    static const std::string SCRIPT_FILENAME_KEY;
    static const std::string DOCUMENT_ROOT_KEY;
    
    static const std::string REMOTE_USER_KEY;
    static const std::string REMOTE_ADDR_KEY;
    
    static const std::string QUERY_STRING_KEY;
    static const std::string REQUEST_METHOD_KEY;
};

const std::string Request::Request_Data::HEAD("HEAD");
const std::string Request::Request_Data::HOST_KEY("HOST");
const std::string Request::Request_Data::CONTENT_TYPE_KEY("Content-Type");
const std::string Request::Request_Data::CONTENT_ENCODING_KEY("Content-Encoding");
const std::string Request::Request_Data::CONTENT_LENGTH_KEY("Content-Length");

const std::string Request::Request_Data::HTTPS_KEY("HTTPS");
const std::string Request::Request_Data::SERVER_ADDR_KEY("SERVER_ADDR");
const std::string Request::Request_Data::SERVER_PORT_KEY("SERVER_PORT");

const std::string Request::Request_Data::PATH_INFO_KEY("PATH_INFO");
const std::string Request::Request_Data::PATH_TRANSLATED_KEY("PATH_TRANSLATED");

const std::string Request::Request_Data::SCRIPT_NAME_KEY("SCRIPT_NAME");
const std::string Request::Request_Data::SCRIPT_FILENAME_KEY("SCRIPT_FILENAME");
const std::string Request::Request_Data::DOCUMENT_ROOT_KEY("DOCUMENT_ROOT");

const std::string Request::Request_Data::REMOTE_USER_KEY("REMOTE_USER");
const std::string Request::Request_Data::REMOTE_ADDR_KEY("REMOTE_ADDR");

const std::string Request::Request_Data::QUERY_STRING_KEY("QUERY_STRING");
const std::string Request::Request_Data::REQUEST_METHOD_KEY("REQUEST_METHOD");

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

Request::Request_Data::Request_Data() : is_bot_(false)
{}

Request::Request_Data::~Request_Data()
{}

Request::Request() : data_(new Request_Data()) {
}

Request::~Request() {
    delete data_;
}

unsigned short
Request::getServerPort() const {
    const std::string &res = Parser::get(data_->vars_, Request_Data::SERVER_PORT_KEY);
    return (!res.empty()) ? boost::lexical_cast<unsigned short>(res) : 80;
}

const std::string&
Request::getServerAddr() const {
    return Parser::get(data_->vars_, Request_Data::SERVER_ADDR_KEY);
}

const std::string&
Request::getPathInfo() const {
    return Parser::get(data_->vars_, Request_Data::PATH_INFO_KEY);
}

const std::string&
Request::getPathTranslated() const {
    return Parser::get(data_->vars_, Request_Data::PATH_TRANSLATED_KEY);
}

const std::string&
Request::getScriptName() const {
    return Parser::get(data_->vars_, Request_Data::SCRIPT_NAME_KEY);
}

const std::string&
Request::getScriptFilename() const {
    return Parser::get(data_->vars_, Request_Data::SCRIPT_FILENAME_KEY);
}

const std::string&
Request::getDocumentRoot() const {
    return Parser::get(data_->vars_, Request_Data::DOCUMENT_ROOT_KEY);
}

const std::string&
Request::getRemoteUser() const {
    return Parser::get(data_->vars_, Request_Data::REMOTE_USER_KEY);
}

const std::string&
Request::getRemoteAddr() const {
    return Parser::get(data_->vars_, Request_Data::REMOTE_ADDR_KEY);
}

const std::string&
Request::getRealIP() const {
    MessageParam<const Request> request_param(this);
    MessageParamBase* param_list[1];
    param_list[0] = &request_param;
    MessageParams params(1, param_list);
    MessageResult<const std::string*> result;
  
    MessageProcessor::instance()->process(REAL_IP_METHOD, params, result);
    return *result.get();
}

const std::string&
Request::getQueryString() const {
    return Parser::get(data_->vars_, Request_Data::QUERY_STRING_KEY);
}

const std::string&
Request::getRequestMethod() const {
    return Parser::get(data_->vars_, Request_Data::REQUEST_METHOD_KEY);
}

std::string
Request::getURI() const {
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
Request::getOriginalURI() const {
    MessageParam<const Request> request_param(this);
    MessageParamBase* param_list[1];
    param_list[0] = &request_param;
    MessageParams params(1, param_list);
    MessageResult<std::string> result;
  
    MessageProcessor::instance()->process(ORIGINAL_URI_METHOD, params, result);
    return result.get();
}

std::string
Request::getOriginalUrl() const {
    std::string url(isSecure() ? "https://" : "http://");
    url.append(getOriginalHost());
    url.append(getOriginalURI());
    return url;
}


const std::string&
Request::getHost() const {
    return getHeader(Request_Data::HOST_KEY);
}

const std::string&
Request::getOriginalHost() const {
    MessageParam<const Request> request_param(this);
    MessageParamBase* param_list[1];
    param_list[0] = &request_param;
    MessageParams params(1, param_list);
    MessageResult<const std::string*> result;
  
    MessageProcessor::instance()->process(ORIGINAL_HOST_METHOD, params, result);
    return *result.get();
}

boost::uint32_t
Request::getContentLength() const {
    const std::string& length = getHeader(Request_Data::CONTENT_LENGTH_KEY);
    if (length.empty()) {
        return 0;
    }
    try {
        return boost::lexical_cast<boost::uint32_t>(length);
    }
    catch (const boost::bad_lexical_cast &e) {
        return 0;
    }
}

const std::string&
Request::getContentType() const {
    return getHeader(Request_Data::CONTENT_TYPE_KEY);
}

const std::string&
Request::getContentEncoding() const {
    return getHeader(Request_Data::CONTENT_ENCODING_KEY);
}

unsigned int
Request::countArgs() const {
    return data_->args_.size();
}

bool
Request::hasArg(const std::string &name) const {
    for(std::vector<StringUtils::NamedValue>::const_iterator i = data_->args_.begin(),
            end = data_->args_.end();
        i != end;
        ++i) {
        if (i->first == name) {
            return true;
        }
    }
    return false;
}

const std::string&
Request::getArg(const std::string &name) const {
    for(std::vector<StringUtils::NamedValue>::const_iterator i = data_->args_.begin(),
            end = data_->args_.end();
        i != end;
        ++i) {
        if (i->first == name) {
            return i->second;
        }
    }
    return StringUtils::EMPTY_STRING;
}

void
Request::getArg(const std::string &name, std::vector<std::string> &v) const {
    std::vector<std::string> tmp;
    tmp.reserve(data_->args_.size());
    for(std::vector<StringUtils::NamedValue>::const_iterator i = data_->args_.begin(),
            end = data_->args_.end();
        i != end;
        ++i) {
        if (i->first == name) {
            tmp.push_back(i->second);
        }
    }
    v.swap(tmp);
}

void
Request::argNames(std::vector<std::string> &v) const {
    std::set<std::string> names;
    v.clear();
    v.reserve(data_->args_.size());
    for(std::vector<StringUtils::NamedValue>::const_iterator i = data_->args_.begin(),
            end = data_->args_.end();
        i != end;
        ++i) {
        std::pair<std::set<std::string>::iterator, bool> result = names.insert(i->first);
        if (result.second) {
            v.push_back(i->first);
        }
    }
}

const std::vector<StringUtils::NamedValue>&
Request::args() const {
    return data_->args_;
}

unsigned int
Request::countHeaders() const {
    boost::mutex::scoped_lock lock(data_->mutex_);
    return data_->headers_.size();
}

bool
Request::hasHeader(const std::string &name) const {
    boost::mutex::scoped_lock lock(data_->mutex_);
    return Parser::has(data_->headers_, name);
}

const std::string&
Request::getHeader(const std::string &name) const {
    boost::mutex::scoped_lock lock(data_->mutex_);
    return Parser::get(data_->headers_, name);
}

void
Request::headerNames(std::vector<std::string> &v) const {
    boost::mutex::scoped_lock lock(data_->mutex_);
    Parser::keys(data_->headers_, v);
}

unsigned int
Request::countCookies() const {
    return data_->cookies_.size();
}

bool
Request::hasCookie(const std::string &name) const {
    return Parser::has(data_->cookies_, name);
}

const std::string&
Request::getCookie(const std::string &name) const {
    return Parser::get(data_->cookies_, name);
}

void
Request::cookieNames(std::vector<std::string> &v) const {
    Parser::keys(data_->cookies_, v);
}

unsigned int
Request::countVariables() const {
    return data_->vars_.size();
}

bool
Request::hasVariable(const std::string &name) const {
    return Parser::has(data_->vars_, name);
}

const std::string&
Request::getVariable(const std::string &name) const {
    return Parser::get(data_->vars_, name);
}

void
Request::variableNames(std::vector<std::string> &v) const {
    Parser::keys(data_->vars_, v);
}

unsigned int
Request::countFiles() const {
    return data_->files_.size();
}

bool
Request::hasFile(const std::string &name) const {
    return data_->files_.find(name) != data_->files_.end();
}

const std::string&
Request::remoteFileName(const std::string &name) const {
    std::map<std::string, File>::const_iterator i = data_->files_.find(name);
    if (data_->files_.end() != i) {
        return i->second.remoteName();
    }
    return StringUtils::EMPTY_STRING;
}

const std::string&
Request::remoteFileType(const std::string &name) const {
    std::map<std::string, File>::const_iterator i = data_->files_.find(name);
    if (data_->files_.end() != i) {
        return i->second.type();
    }
    return StringUtils::EMPTY_STRING;
}

std::pair<const char*, std::streamsize>
Request::remoteFile(const std::string &name) const {
    std::map<std::string, File>::const_iterator i = data_->files_.find(name);
    if (data_->files_.end() != i) {
        return i->second.data();
    }
    return std::make_pair<const char*, std::streamsize>(NULL, 0);
}

void
Request::fileNames(std::vector<std::string> &v) const {
    Parser::keys(data_->files_, v);
}

bool
Request::isSecure() const {
    const std::string &val = Parser::get(data_->vars_, Request_Data::HTTPS_KEY);
    return !val.empty() && strncasecmp(val.c_str(), "on", sizeof("on")) == 0;
}

bool
Request::isBot() const {
    return data_->is_bot_;
}

std::pair<const char*, std::streamsize>
Request::requestBody() const {
    return std::make_pair<const char*, std::streamsize>(&data_->body_[0], data_->body_.size());
}

bool
Request::suppressBody() const {
    return Request_Data::HEAD == getRequestMethod();
}

void
Request::addInputHeader(const std::string &name, const std::string &value) {
    Range key_range = createRange(name);
    Range value_range = createRange(value);
    std::auto_ptr<Encoder> enc = Encoder::createDefault("cp1251", "UTF-8");
    boost::mutex::scoped_lock lock(data_->mutex_);
    Parser::addHeader(this, key_range, value_range, enc.get());
}

void
Request::attach(std::istream *is, char *env[]) {
    try {
        MessageParam<Request> request_param(this);
        MessageParam<std::istream> stream_param(is);
        MessageParam<char*> env_param(env);
        
        MessageParamBase* param_list[3];
        param_list[0] = &request_param;
        param_list[1] = &stream_param;
        param_list[2] = &env_param;
        
        MessageParams params(3, param_list);
        MessageResultBase result;
      
        MessageProcessor::instance()->process(ATTACH_METHOD, params, result);
    }
    catch(const BadRequestError &e) {
        throw;
    }
    catch(const std::exception &e) {
        throw BadRequestError(e.what());
    }
    catch(...) {
        throw BadRequestError("Unknown error");
    }
}

const Range Parser::RN_RANGE = createRange("\r\n");
const Range Parser::NAME_RANGE = createRange("name");
const Range Parser::FILENAME_RANGE = createRange("filename");
const Range Parser::HEADER_RANGE = createRange("HTTP_");
const Range Parser::COOKIE_RANGE = createRange("HTTP_COOKIE");
const Range Parser::EMPTY_LINE_RANGE = createRange("\r\n\r\n");
const Range Parser::CONTENT_TYPE_RANGE = createRange("CONTENT_TYPE");
const Range Parser::CONTENT_TYPE_MULTIPART_RANGE = createRange("Content-Type");

const std::string Parser::NORMALIZE_HEADER_METHOD = "PARSER_NORMALIZE_HEADER_METHOD";

const char*
Parser::statusToString(short status) {

    switch (status) {
    case 200:
        return "OK";
    case 201:
        return "Created";
    case 202:
        return "Accepted";
    case 203:
        return "Non-Authoritative Information";
    case 204:
        return "No Content";

    case 301:
        return "Moved Permanently";
    case 302:
        return "Found";
    case 303:
        return "See Other";
    case 304:
        return "Not Modified";

    case 400:
        return "Bad request";
    case 401:
        return "Unauthorized";
    case 402:
        return "Payment Required";
    case 403:
        return "Forbidden";
    case 404:
        return "Not Found";
    case 405:
        return "Method Not Allowed";

    case 500:
        return "Internal Server Error";
    case 501:
        return "Not Implemented";
    case 502:
        return "Bad Gateway";
    case 503:
        return "Service Unavailable";
    case 504:
        return "Gateway Timeout";
    }
    return "Unknown status";
}

std::string
Parser::getBoundary(const Range &range) {

    Range head, tail;
    split(range, ';', head, tail);

    tail = trim(tail);

    if (strncasecmp("boundary", tail.begin(), sizeof("boundary") - 1) == 0) {
        Range key, value;
        split(tail, '=', key, value);
        Range boundary = trim(value);

        Range comma = createRange("\"");
        if (startsWith(boundary, comma) && endsWith(boundary, comma)) {
            return std::string("--").append(boundary.begin() + 1, boundary.end() - 1);
        }

        return std::string("--").append(boundary.begin(), boundary.end());
    }
    throw std::runtime_error("no boundary found");
}

void
Parser::addCookie(Request *req, const Range &range, Encoder *encoder) {

    Range part = trim(range), head, tail;
    split(part, '=', head, tail);
    if (!head.empty()) {
        std::string key = StringUtils::urldecode(head), value = StringUtils::urldecode(tail);
        if (!xmlCheckUTF8((const xmlChar*) key.c_str())) {
            encoder->encode(key).swap(key);
        }
        if (!xmlCheckUTF8((const xmlChar*) value.c_str())) {
            encoder->encode(value).swap(value);
        }
        req->data_->cookies_[key].swap(value);
    }
}

void
Parser::addHeader(Request *req, const Range &key, const Range &value, Encoder *encoder) {
    std::string header = normalizeInputHeaderName(key);
    
    Range checked_value = value;
    if (strcmp(header.c_str(), Request::Request_Data::HOST_KEY.c_str()) == 0) {
        checked_value = checkHost(value);
    }
        
    Range norm_value = checked_value;
    std::string result;
    if (normalizeHeader(header, checked_value, result)) {
        norm_value = createRange(result);
    }
    
    if (!xmlCheckUTF8((const xmlChar*) norm_value.begin())) {
        encoder->encode(norm_value).swap(req->data_->headers_[header]);
    }
    else {
        std::string &str = req->data_->headers_[header];
        str.reserve(norm_value.size());
        str.assign(norm_value.begin(), norm_value.end());
    }
}

void
Parser::parse(Request *req, char *env[], Encoder *encoder) {
    for (int i = 0; NULL != env[i]; ++i) {
        log()->debug("env[%d] = %s", i, env[i]);
        Range key, value;
        split(createRange(env[i]), '=', key, value);
        if (COOKIE_RANGE == key) {
            parseCookies(req, value, encoder);
            addHeader(req, truncate(key, HEADER_RANGE.size(), 0), trim(value), encoder);
        }
        else if (CONTENT_TYPE_RANGE == key) {
            addHeader(req, key, trim(value), encoder);
        }
        else if (startsWith(key, HEADER_RANGE)) { 
            addHeader(req, truncate(key, HEADER_RANGE.size(), 0), trim(value), encoder);
        }
        else {
            std::string name(key.begin(), key.end());
            std::string query_value;
            if (strcmp(name.c_str(), Request::Request_Data::QUERY_STRING_KEY.c_str()) == 0) {
                query_value = checkUrlEscaping(value);
                value = createRange(query_value);
            }
            
            if (!xmlCheckUTF8((const xmlChar*) value.begin())) {
                encoder->encode(value).swap(req->data_->vars_[name]);
            }
            else {
                std::string &str = req->data_->vars_[name];
                str.reserve(value.size());
                str.assign(value.begin(), value.end());
            }
        }
    }
}

void
Parser::parseCookies(Request *req, const Range &range, Encoder *encoder) {
    Range part = trim(range), head, tail;
    while (!part.empty()) {
        split(part, ';', head, tail);
        addCookie(req, head, encoder);
        part = trim(tail);
    }
}

void
Parser::parseLine(Range &line, std::map<Range, Range, RangeCILess> &m) {

    Range head, tail, name, value;
    while (!line.empty()) {
        split(line, ';', head, tail);        
        if (head.size() >= CONTENT_TYPE_MULTIPART_RANGE.size() &&
            strncasecmp(head.begin(),
                    CONTENT_TYPE_MULTIPART_RANGE.begin(),
                    CONTENT_TYPE_MULTIPART_RANGE.size()) == 0) {
            split(head, ':', name, value);
            value = trim(value);
        }
        else {
            split(head, '=', name, value);
            if (NAME_RANGE == name || FILENAME_RANGE == name) {
                value = truncate(value, 1, 1);
            }
        }
        m.insert(std::make_pair(name, value));
        line = trim(tail);
    }
}

void
Parser::parsePart(Request *req, Range &part, Encoder *encoder) {

    Range headers, content, line, tail;
    std::map<Range, Range, RangeCILess> params;

    split(part, EMPTY_LINE_RANGE, headers, content);
    while (!headers.empty()) {
        split(headers, RN_RANGE, line, tail);
        parseLine(line, params);
        headers = tail;
    }
    std::map<Range, Range, RangeCILess>::iterator i = params.find(NAME_RANGE);
    if (params.end() == i) {
        return;
    }

    std::string name(i->second.begin(), i->second.end());
    if (!xmlCheckUTF8((const xmlChar*) name.c_str())) {
        encoder->encode(name).swap(name);
    }

    if (params.end() != params.find(FILENAME_RANGE)) {
        req->data_->files_.insert(std::make_pair(name, File(params, content)));
    }
    else {
        std::pair<std::string, std::string> p;
        p.first.swap(name);
        p.second.assign(content.begin(), content.end());
        if (!xmlCheckUTF8((const xmlChar*) p.second.c_str())) {
            encoder->encode(p.second).swap(p.second);
        }
        req->data_->args_.push_back(p);
    }
}

void
Parser::parseMultipart(Request *req, Range &data, const std::string &boundary, Encoder *encoder) {
    Range head, tail, bound = createRange(boundary);
    while (!data.empty()) {
        split(data, bound, head, tail);
        if (!head.empty()) {
            head = truncate(head, 2, 2);
        }
        if (!head.empty()) {
            parsePart(req, head, encoder);
        }
        data = tail;
    }
}

std::string
Parser::normalizeInputHeaderName(const Range &range) {

    std::string res;
    res.reserve(range.size());

    Range part = range, head, tail;
    while (!part.empty()) {
        bool splitted = split(part, '_', head, tail);
        res.append(head.begin(), head.end());
        if (splitted) {
            res.append(1, '-');
        }
        part = tail;
    }
    return res;
}

std::string
Parser::normalizeOutputHeaderName(const std::string &name) {

    std::string res;
    Range range = trim(createRange(name)), head, tail;
    res.reserve(range.size());

    while (!range.empty()) {

        split(range, '-', head, tail);
        if (!head.empty()) {
            res.append(1, toupper(*head.begin()));
        }
        if (head.size() > 1) {
            res.append(head.begin() + 1, head.end());
        }
        range = trim(tail);
        if (!range.empty()) {
            res.append(1, '-');
        }
    }
    return res;
}

std::string
Parser::normalizeQuery(const Range &range) {
    std::string result;
    unsigned int length = range.size();
    const char *query = range.begin();
    for(unsigned int i = 0; i < length; ++i, ++query) {
        if (static_cast<unsigned char>(*query) > 127) {
            result.append(StringUtils::urlencode(Range(query, query + 1)));
        }
        else {
            result.push_back(*query);
        }
    }
    return result;
}

bool
Parser::normalizeHeader(const std::string &name, const Range &value, std::string &result) {   
    MessageParam<const std::string> name_param(&name);
    MessageParam<const Range> value_param(&value);
    MessageParam<std::string> result_param(&result);
    
    MessageParamBase* param_list[3];
    param_list[0] = &name_param;
    param_list[1] = &value_param;
    param_list[2] = &result_param;
    
    MessageParams params(3, param_list);
    MessageResult<bool> res;
  
    MessageProcessor::instance()->process(NORMALIZE_HEADER_METHOD, params, res);
    return res.get();
}

std::string
Parser::checkUrlEscaping(const Range &range) {
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
Parser::checkHost(const Range &range) {
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

class Request::AttachHandler : public MessageHandler {
    Result process(const MessageParams &params, MessageResultBase &result) {
        (void)result;
        Request* request = params.getPtr<Request>(0);
        std::istream* is = params.getPtr<std::istream>(1);
        char** env = params.getPtr<char*>(2);
        
        std::auto_ptr<Encoder> enc = Encoder::createDefault("cp1251", "UTF-8");
        Parser::parse(request, env, enc.get());

        if (is && "POST" == request->getRequestMethod()) {
            request->data_->body_.resize(request->getContentLength());
            is->exceptions(std::ios::badbit);
            is->read(&request->data_->body_[0], request->data_->body_.size());
            
            if (is->gcount() != static_cast<std::streamsize>(request->data_->body_.size())) {
                throw std::runtime_error("failed to read request entity");
            }
            
            const std::string &type = request->getContentType();
            if (strncasecmp("multipart/form-data", type.c_str(), sizeof("multipart/form-data") - 1) == 0) {
                Range body = createRange(request->data_->body_);
                std::string boundary = Parser::getBoundary(createRange(type));
                Parser::parseMultipart(request, body, boundary, enc.get());
            }
            else if (!request->data_->body_.empty()) {
                StringUtils::parse(createRange(request->data_->body_), request->data_->args_, enc.get());
            }
        }
        else {
            const std::string &query = request->getQueryString();
            if (!query.empty()) {
                StringUtils::parse(createRange(query), request->data_->args_, enc.get());
            }
        }
        request->data_->is_bot_ = Authorizer::instance()->isBot(request->getHeader("User-Agent"));
        return CONTINUE;
    }
};

class Request::RealIPHandler : public MessageHandler {
    Result process(const MessageParams &params, MessageResultBase &result) {
        const Request* request = params.getPtr<const Request>(0);
        result.set(&request->getRemoteAddr());
        return CONTINUE;
    }
};

class Request::OriginalURIHandler : public MessageHandler {
    Result process(const MessageParams &params, MessageResultBase &result) {
        const Request* request = params.getPtr<const Request>(0);
        result.set(request->getURI());
        return CONTINUE;
    }
};

class Request::OriginalHostHandler : public MessageHandler {
    Result process(const MessageParams &params, MessageResultBase &result) {
        const Request* request = params.getPtr<const Request>(0);
        result.set(&request->getHost());
        return CONTINUE;
    }
};

class NormalizeHeaderHandler : public MessageHandler {
    Result process(const MessageParams &params, MessageResultBase &result) {
        (void)params;
        result.set(false);
        return CONTINUE;
    }
};

class Request::HandlerRegisterer {
public:
    HandlerRegisterer() {
        MessageProcessor::instance()->registerBack(ATTACH_METHOD,
            boost::shared_ptr<MessageHandler>(new AttachHandler()));
        MessageProcessor::instance()->registerBack(REAL_IP_METHOD,
            boost::shared_ptr<MessageHandler>(new RealIPHandler()));
        MessageProcessor::instance()->registerBack(ORIGINAL_URI_METHOD,
            boost::shared_ptr<MessageHandler>(new OriginalURIHandler()));
        MessageProcessor::instance()->registerBack(ORIGINAL_HOST_METHOD,
            boost::shared_ptr<MessageHandler>(new OriginalHostHandler()));        
    }
};

class ParserHandlerRegisterer {
public:
    ParserHandlerRegisterer() {
        MessageProcessor::instance()->registerBack(Parser::NORMALIZE_HEADER_METHOD,
            boost::shared_ptr<MessageHandler>(new NormalizeHeaderHandler()));
    }
};

static Request::HandlerRegisterer reg_request_handlers;
static ParserHandlerRegisterer reg_parser_handlers;

} // namespace xscript
