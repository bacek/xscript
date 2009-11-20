#include "settings.h"

#include <sstream>

#include <boost/current_function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/mutex.hpp>

#include "xscript/logger.h"
#include "xscript/http_utils.h"
#include "xscript/request.h"
#include "xscript/response.h"
#include "xscript/vhost_data.h"
#include "xscript/writer.h"

#include "internal/parser.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class Response::ResponseData {
public:
    ResponseData(Response *response);
    ~ResponseData();
    
    void sendHeadersInternal();
    bool isBinaryInternal() const;
    
    Response *response_;

    HeaderMap out_headers_;
    CookieSet out_cookies_;
    
    std::auto_ptr<BinaryWriter> writer_;

    bool headers_sent_;
    unsigned short status_;
    bool detached_, stream_locked_;
    mutable boost::mutex write_mutex_, resp_mutex_;
};

Response::ResponseData::ResponseData(Response *response) :
    response_(response), writer_(NULL), headers_sent_(false),
    status_(200), detached_(false), stream_locked_(false)
{}

Response::ResponseData::~ResponseData()
{}

void
Response::ResponseData::sendHeadersInternal() {
    boost::mutex::scoped_lock sl(resp_mutex_);
    std::stringstream stream;
    if (!headers_sent_) {
        log()->debug("%s, sending headers", BOOST_CURRENT_FUNCTION);
        stream << status_ << " " << HttpUtils::statusToString(status_);
        out_headers_["Status"] = stream.str();
        response_->writeHeaders();
        headers_sent_ = true;
    }
}

bool
Response::ResponseData::isBinaryInternal() const {
    return NULL != writer_.get();
}

Response::Response() : data_(new ResponseData(this)){
}

Response::~Response() {
    delete data_;
}

void
Response::setCookie(const Cookie &cookie) {
    if (!cookie.check()) {
        const Request *request = VirtualHostData::instance()->get();
        std::string url = request ? request->getOriginalUrl() : StringUtils::EMPTY_STRING;
        log()->warn("Incorrect cookie. Skipped. Url: %s Name: %s; value: %s; domain: %s; path: %s", url.c_str(),
                StringUtils::urlencode(cookie.name()).c_str(), StringUtils::urlencode(cookie.value()).c_str(),
                StringUtils::urlencode(cookie.domain()).c_str(), StringUtils::urlencode(cookie.path()).c_str());
        return;
    }
    boost::mutex::scoped_lock sl(data_->resp_mutex_);
    if (!data_->headers_sent_) {
        data_->out_cookies_.insert(cookie);
    }
    else {
        throw std::runtime_error("headers already sent");
    }
}

void
Response::setStatus(unsigned short status) {
    boost::mutex::scoped_lock sl(data_->resp_mutex_);
    if (!data_->headers_sent_) {
        data_->status_ = status;
    }
    else {
        throw std::runtime_error("headers already sent");
    }
}

void
Response::sendError(unsigned short status, const std::string &message) {
    log()->debug("%s, clearing request output", BOOST_CURRENT_FUNCTION);
    {
        boost::mutex::scoped_lock sl(data_->resp_mutex_);
        if (!data_->headers_sent_) {
            data_->out_cookies_.clear();
            data_->out_headers_.clear();
        }
        else {
            throw std::runtime_error("headers already sent");
        }
        data_->status_ = status;
        data_->out_headers_.insert(std::pair<std::string, std::string>("Content-type", "text/html"));
    }

    boost::mutex::scoped_lock sl(data_->write_mutex_);
    if (data_->detached_) {
        return;
    }

    if (data_->stream_locked_ || data_->isBinaryInternal()) {
        throw std::runtime_error("Cannot write data. Output stream is already occupied");
    }
    data_->sendHeadersInternal();
    writeError(status, message);
}

void
Response::setHeader(const std::string &name, const std::string &value) {
    boost::mutex::scoped_lock sl(data_->resp_mutex_);
    if (data_->headers_sent_) {
        throw std::runtime_error("headers already sent");
    }
    std::string normalized_name = HttpUtils::normalizeOutputHeaderName(name);
    std::string::size_type pos = value.find_first_of("\r\n");
    if (pos == std::string::npos) {
        data_->out_headers_[normalized_name] = value;
    }
    else {
        data_->out_headers_[normalized_name].assign(value.begin(), value.begin() + pos);
    }
}

std::streamsize
Response::write(const char *buf, std::streamsize size, Request *request) {
    boost::mutex::scoped_lock wl(data_->write_mutex_);
    if (data_->isBinaryInternal()) {
        throw std::runtime_error("Cannot write data. Output stream is already occupied");
    }

    data_->stream_locked_ = true;
    data_->sendHeadersInternal();
    if (!request->suppressBody()) {
        if (data_->detached_) {
            return 0;
        }
        writeBuffer(buf, size);
    }
    return size;
}

std::streamsize
Response::write(std::auto_ptr<BinaryWriter> writer) {
    boost::mutex::scoped_lock wl(data_->write_mutex_);
    if (data_->detached_) {
        return 0;
    }

    if (data_->stream_locked_ || data_->isBinaryInternal()) {
        throw std::runtime_error("Cannot write data. Output stream is already occupied");
    }

    data_->writer_ = writer;
    return data_->writer_->size();
}

std::string
Response::outputHeader(const std::string &name) const {
    boost::mutex::scoped_lock sl(data_->resp_mutex_);
    return Parser::get(data_->out_headers_, name);
}

void
Response::sendHeaders() {
    boost::mutex::scoped_lock sl(data_->write_mutex_);
    data_->sendHeadersInternal();
}

void
Response::detach() {
    boost::mutex::scoped_lock wl(data_->write_mutex_);
    if (!data_->stream_locked_ && data_->isBinaryInternal()) {   
        setHeader("Content-length", boost::lexical_cast<std::string>(data_->writer_->size()));
        data_->sendHeadersInternal();
        writeByWriter(data_->writer_.get());
    }
    else {
        data_->sendHeadersInternal();
    }
    data_->detached_ = true;
}

bool
Response::suppressBody(const Request *req) const {
    return req->suppressBody() || 204 == data_->status_ || 304 == data_->status_;
}

void
Response::writeBuffer(const char *buf, std::streamsize size) {
    (void)buf;
    (void)size;
}

void
Response::writeByWriter(const BinaryWriter *writer) {
    (void)writer;
}

void
Response::writeError(unsigned short status, const std::string &message) {
    (void)status;
    (void)message;
}

void
Response::writeHeaders() {
}

const HeaderMap&
Response::outHeaders() const {
    return data_->out_headers_;
}
const CookieSet&
Response::outCookies() const {
    return data_->out_cookies_;
}

bool
Response::isBinary() const {
    boost::mutex::scoped_lock wl(data_->write_mutex_);
    return data_->isBinaryInternal();
}

unsigned short
Response::status() const {
    boost::mutex::scoped_lock sl(data_->resp_mutex_);
    return data_->status_;
}

void
Response::redirectBack(const Request *req) {
    redirectToPath(req->getHeader("Referer"));
}

void
Response::redirectToPath(const std::string &path) {
    setStatus(302);
    setHeader("Location", path);
}

void
Response::setContentType(const std::string &type) {
    setHeader("Content-type", type);
}

void
Response::setContentEncoding(const std::string &encoding) {
    setHeader("Content-encoding", encoding);
}



} // namespace xscript
