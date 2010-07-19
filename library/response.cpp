#include "settings.h"

#include <sstream>

#include <boost/current_function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/mutex.hpp>

#include "xscript/context.h"
#include "xscript/doc_cache.h"
#include "xscript/http_utils.h"
#include "xscript/logger.h"
#include "xscript/request.h"
#include "xscript/response.h"
#include "xscript/script.h"
#include "xscript/tag.h"
#include "xscript/vhost_data.h"
#include "xscript/writer.h"
#include "xscript/xml_util.h"

#include "internal/parser.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

static const boost::uint32_t DEFAULT_EXPIRE_TIME_DELTA = 300;

class Response::ResponseData {
public:
    ResponseData(Response *response, std::ostream *stream);
    ~ResponseData();
    
    void sendHeaders();
    bool isBinary() const;
    bool cacheable() const;
    
    Response *response_;

    std::ostream *stream_;
    
    HeaderMap out_headers_;
    CookieSet out_cookies_;
    
    std::auto_ptr<BinaryWriter> writer_;

    bool headers_sent_;
    unsigned short status_;
    bool detached_, stream_locked_;
    mutable boost::mutex write_mutex_, resp_mutex_;
    boost::shared_ptr<PageCacheData> cache_data_;
    bool have_cached_copy_;
    bool suppress_body_;
    boost::uint32_t expire_delta_;
};

Response::ResponseData::ResponseData(Response *response, std::ostream *stream) :
    response_(response), stream_(stream), writer_(NULL), headers_sent_(false), status_(200),
    detached_(false), stream_locked_(false), have_cached_copy_(false),
    suppress_body_(false), expire_delta_(DEFAULT_EXPIRE_TIME_DELTA)
{}

Response::ResponseData::~ResponseData()
{}

void
Response::ResponseData::sendHeaders() {
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
Response::ResponseData::isBinary() const {
    return NULL != writer_.get();
}

bool
Response::ResponseData::cacheable() const {
    return NULL != cache_data_.get();
}

Response::Response() : data_(new ResponseData(this, NULL)) {
}

Response::Response(std::ostream *stream) :
    data_(new ResponseData(this, stream)) {
    stream->exceptions(std::ios::badbit);
}

Response::~Response() {
}

void
Response::setCookie(const Cookie &cookie) {
    if (!cookie.check()) {
        const Request *request = VirtualHostData::instance()->get();
        std::string url = request ? request->getOriginalUrl() : StringUtils::EMPTY_STRING;
        log()->warn("Incorrect cookie. Skipped. Url: %s Name: %s; value: %s; domain: %s; path: %s",
                url.c_str(),
                StringUtils::urlencode(cookie.name()).c_str(),
                StringUtils::urlencode(cookie.value()).c_str(),
                StringUtils::urlencode(cookie.domain()).c_str(),
                StringUtils::urlencode(cookie.path()).c_str());
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

    if (data_->stream_locked_ || data_->isBinary()) {
        throw std::runtime_error("Cannot write data. Output stream is already occupied");
    }
    data_->sendHeaders();
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

void
Response::setExpiresHeader() {
    boost::int32_t expires = HttpDateUtils::expires(expireDelta());
    setHeader(HttpUtils::EXPIRES_HEADER_NAME, HttpDateUtils::format(expires));
}

void
Response::setExpireDelta(boost::uint32_t delta) {
    boost::mutex::scoped_lock sl(data_->resp_mutex_);
    data_->expire_delta_ = delta;
}

void
Response::setSuppressBody(bool value) {
    boost::mutex::scoped_lock sl(data_->resp_mutex_);
    data_->suppress_body_ = value;
}

std::streamsize
Response::write(const char *buf, std::streamsize size, Request *request) {
    boost::mutex::scoped_lock wl(data_->write_mutex_);
    if (data_->isBinary()) {
        throw std::runtime_error("Cannot write data. Output stream is already occupied");
    }

    if (!data_->cacheable()) {
        data_->stream_locked_ = true;
    }
    data_->sendHeaders();
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

    if (data_->stream_locked_ || data_->isBinary()) {
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
    data_->sendHeaders();
}

void
Response::detach(Context *ctx) {
    boost::mutex::scoped_lock wl(data_->write_mutex_);
    bool cacheable = data_->cacheable();
    
    if (data_->stream_locked_) {
        data_->sendHeaders();
    }
    else if (cacheable) {
        if (data_->have_cached_copy_ && PageCache::instance()->useETag()) {
            setStatus(304);
            setExpiresHeader();
            setHeader(HttpUtils::ETAG_HEADER_NAME, data_->cache_data_->etag());
            data_->cache_data_ = boost::shared_ptr<PageCacheData>(); //need for sending headers in non-cache mode
            data_->sendHeaders();
        }
        else {
            writeByWriter(data_->cache_data_.get());
        }
    }
    else if (data_->isBinary()) {
        setHeader("Content-length", boost::lexical_cast<std::string>(data_->writer_->size()));
        data_->sendHeaders();
        writeByWriter(data_->writer_.get());
    }
    else {
        data_->sendHeaders();
    }

    data_->detached_ = true;
    
    (*data_->stream_) << std::flush;
    
    if (cacheable && !data_->have_cached_copy_ && ctx) {
        try {
            Tag tag;
            Script* script = ctx->script().get();
            tag.expire_time = time(NULL) + script->cacheTime();
            CacheContext cache_ctx(script, ctx, script->allowDistributed());
            PageCache::instance()->saveDoc(&cache_ctx, tag, data_->cache_data_);
        }
        catch(const std::exception &e) {
            log()->error("Error in saving page to cache: %s", e.what());
        }
    }
}

bool
Response::suppressBody(const Request *req) const {
    boost::mutex::scoped_lock sl(data_->resp_mutex_);
    return data_->suppress_body_ || req->suppressBody() ||
        204 == data_->status_ || 304 == data_->status_;
}

void
Response::writeBuffer(const char *buf, std::streamsize size) {
    if (data_->cacheable()) {
        data_->cache_data_->append(buf, size);
    }
    else {
        data_->stream_->write(buf, size);
    }
}

void
Response::writeByWriter(const BinaryWriter *writer) {
    writer->write(data_->stream_, this);
}

void
Response::writeError(unsigned short status, const std::string &message) {
    (*data_->stream_) << "<html><body><h1>" << status << " " << HttpUtils::statusToString(status) << "<br><br>"
    << XmlUtils::escape(createRange(message)) << "</h1></body></html>";
}

void
Response::writeHeaders() {
    bool cacheable = data_->cacheable();
    const HeaderMap& headers = outHeaders();
    for (HeaderMap::const_iterator i = headers.begin(), end = headers.end(); i != end; ++i) {
        if (cacheable) {
            if (0 == strcasecmp(i->first.c_str(), "expires") ||
                0 == strcasecmp(i->first.c_str(), "etag")) {
                continue;
            }
            data_->cache_data_->addHeader(i->first, i->second);
        }
        else {
            (*data_->stream_) << i->first << ": " << i->second << "\r\n";
        }
    }

    if (!cacheable) {
        const CookieSet& cookies = outCookies();
        for (CookieSet::const_iterator i = cookies.begin(), end = cookies.end(); i != end; ++i) {
            (*data_->stream_) << "Set-Cookie: " << i->toString() << "\r\n";
        }
        (*data_->stream_) << "\r\n";
    }
}

const HeaderMap&
Response::outHeaders() const {
    return data_->out_headers_;
}
const CookieSet&
Response::outCookies() const {
    return data_->out_cookies_;
}

boost::uint32_t
Response::expireDelta() const {
    boost::mutex::scoped_lock sl(data_->resp_mutex_);
    return data_->expire_delta_;
}

bool
Response::isBinary() const {
    boost::mutex::scoped_lock wl(data_->write_mutex_);
    return data_->isBinary();
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

void
Response::setCacheable(boost::shared_ptr<PageCacheData> cache_data) {
    if (NULL != cache_data.get()) {
        data_->have_cached_copy_ = true;
        data_->cache_data_ = cache_data;
    }
    else {
        data_->cache_data_ = boost::shared_ptr<PageCacheData>(new PageCacheData());
    }
    data_->cache_data_->expireTimeDelta(expireDelta());
}

ResponseDetacher::ResponseDetacher(Response *resp, const boost::shared_ptr<Context> &ctx) :
    resp_(resp), ctx_(ctx) {
    assert(NULL != resp);
}

ResponseDetacher::~ResponseDetacher() {
    resp_->detach(ctx_.get());
}

} // namespace xscript
