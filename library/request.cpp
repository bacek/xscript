#include "settings.h"

#include <map>
#include <stdexcept>

#include <boost/lexical_cast.hpp>

#include "internal/parser.h"
#include "internal/request_impl.h"

#include "xscript/authorizer.h"
#include "xscript/encoder.h"
#include "xscript/functors.h"
#include "xscript/http_utils.h"
#include "xscript/logger.h"
#include "xscript/message_interface.h"
#include "xscript/range.h"
#include "xscript/request.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

const std::string Request::ATTACH_METHOD = "REQUEST_ATTACH";
const std::string Request::REAL_IP_METHOD = "REQUEST_REAL_IP";
const std::string Request::ORIGINAL_URI_METHOD = "REQUEST_ORIGINAL_URI";
const std::string Request::ORIGINAL_HOST_METHOD = "REQUEST_ORIGINAL_HOST";

Request::Request() : data_(new RequestImpl()) {
}

Request::~Request() {
}

unsigned short
Request::getServerPort() const {
    const std::string &res = Parser::get(data_->vars_, RequestImpl::SERVER_PORT_KEY);
    return (!res.empty()) ? boost::lexical_cast<unsigned short>(res) : 80;
}

const std::string&
Request::getServerAddr() const {
    return Parser::get(data_->vars_, RequestImpl::SERVER_ADDR_KEY);
}

const std::string&
Request::getPathInfo() const {
    return Parser::get(data_->vars_, RequestImpl::PATH_INFO_KEY);
}

const std::string&
Request::getPathTranslated() const {
    return Parser::get(data_->vars_, RequestImpl::PATH_TRANSLATED_KEY);
}

const std::string&
Request::getScriptName() const {
    return Parser::get(data_->vars_, RequestImpl::SCRIPT_NAME_KEY);
}

const std::string&
Request::getScriptFilename() const {
    return Parser::get(data_->vars_, RequestImpl::SCRIPT_FILENAME_KEY);
}

const std::string&
Request::getDocumentRoot() const {
    return Parser::get(data_->vars_, RequestImpl::DOCUMENT_ROOT_KEY);
}

const std::string&
Request::getRemoteUser() const {
    return Parser::get(data_->vars_, RequestImpl::REMOTE_USER_KEY);
}

const std::string&
Request::getRemoteAddr() const {
    return Parser::get(data_->vars_, RequestImpl::REMOTE_ADDR_KEY);
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
    return Parser::get(data_->vars_, RequestImpl::QUERY_STRING_KEY);
}

const std::string&
Request::getRequestMethod() const {
    return Parser::get(data_->vars_, RequestImpl::REQUEST_METHOD_KEY);
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

const std::string&
Request::getRequestURI() const {
    return Parser::get(data_->vars_, RequestImpl::REQUEST_URI_KEY);
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
    return getHeader(RequestImpl::HOST_KEY);
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
    const std::string& length = getHeader(RequestImpl::CONTENT_LENGTH_KEY);
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
    return getHeader(RequestImpl::CONTENT_TYPE_KEY);
}

const std::string&
Request::getContentEncoding() const {
    return getHeader(RequestImpl::CONTENT_ENCODING_KEY);
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
        end = data_->args_.end(); i != end; ++i) {
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
    const std::string &val = Parser::get(data_->vars_, RequestImpl::HTTPS_KEY);
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
    return RequestImpl::HEAD == getRequestMethod();
}

void
Request::addInputHeader(const std::string &name, const std::string &value) {
    Range key_range = createRange(name);
    Range value_range = createRange(value);
    std::auto_ptr<Encoder> enc = Encoder::createDefault("cp1251", "UTF-8");
    boost::mutex::scoped_lock lock(data_->mutex_);
    Parser::addHeader(data_.get(), key_range, value_range, enc.get());
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

class Request::AttachHandler : public MessageHandler {
    Result process(const MessageParams &params, MessageResultBase &result) {
        (void)result;
        Request* request = params.getPtr<Request>(0);
        std::istream* is = params.getPtr<std::istream>(1);
        char** env = params.getPtr<char*>(2);
        
        std::auto_ptr<Encoder> enc = Encoder::createDefault("cp1251", "UTF-8");
        Parser::parse(request->data_.get(), env, enc.get());

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
                Parser::parseMultipart(request->data_.get(), body, boundary, enc.get());
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

        if (request->getHeader(HttpUtils::ACCEPT_HEADER_NAME).empty()) {
            log()->info("Bot detected with empty accept header");
            request->data_->is_bot_ = true;
        }
        else {
            request->data_->is_bot_ = Authorizer::instance()->isBot(
                request->getHeader(HttpUtils::USER_AGENT_HEADER_NAME));
        }

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
        const std::string& uri = request->getRequestURI();
        if (uri.empty()) {
            result.set(request->getURI());
        }
        else {
            result.set(uri);
        }
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

static Request::HandlerRegisterer reg_request_handlers;

} // namespace xscript
