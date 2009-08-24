#include "settings.h"

#include <cerrno>
#include <cstdlib>
#include <sstream>
#include <iterator>
#include <stdexcept>
#include <algorithm>
#include <map>

#include <boost/current_function.hpp>
#include <boost/tokenizer.hpp>
#include <boost/thread/once.hpp>

#include "xscript/http_helper.h"
#include "xscript/policy.h"
#include "xscript/util.h"
#include "xscript/string_utils.h"
#include "xscript/range.h"
#include "xscript/logger.h"
#include "xscript/remote_tagged_block.h"
#include "xscript/request.h"
#include "internal/algorithm.h"

#include <curl/curl.h>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class HeadersHelper : private boost::noncopyable {
public:
    HeadersHelper() : headers_(NULL) {
    }
    
    ~HeadersHelper() {
        clear();
    }
  
    const curl_slist* get() const {
        return headers_;
    }

    void append(const char *header) {
        headers_ = curl_slist_append(headers_, header);
        log()->debug("%s, added %s", BOOST_CURRENT_FUNCTION, header);
    }
    
    void clear() {
        if (NULL == headers_) {
            return;
        }
        curl_slist_free_all(headers_);
        headers_ = NULL;
    }
private:
    curl_slist *headers_;
};


class HttpHelper::HelperData {
public:
    HelperData(const std::string &url, long timeout) :
        curl_(NULL), status_(0), url_(url), content_(new std::string),
        sent_modified_since_(false) {
        curl_ = curl_easy_init();
        if (NULL != curl_) {
            if (timeout > 0) {
#if (LIBCURL_VERSION_MAJOR > 7) || ((LIBCURL_VERSION_MAJOR == 7) && (LIBCURL_VERSION_MINOR >= 16))
                setopt(CURLOPT_TIMEOUT_MS, timeout);
#else
                setopt(CURLOPT_TIMEOUT, (timeout + 999) / 1000);
#endif
            }
            setopt(CURLOPT_URL, url_.c_str());

            setopt(CURLOPT_NOSIGNAL, static_cast<long>(1));
            setopt(CURLOPT_NOPROGRESS, static_cast<long>(1));
            setopt(CURLOPT_FORBID_REUSE, static_cast<long>(1));

            setopt(CURLOPT_WRITEDATA, &*content_);
            setopt(CURLOPT_WRITEFUNCTION, &curlWrite);
            setopt(CURLOPT_WRITEHEADER, &headers_);
            setopt(CURLOPT_HEADERFUNCTION, &curlHeaders);
        }
    }
    
    ~HelperData() {
        headers_helper_.clear();
        if (NULL != curl_) {
            curl_easy_cleanup(curl_);
        }
    }
    
    template<typename T> void
    setopt(CURLoption opt, T t) {
        check(curl_easy_setopt(curl_, opt, t));
    }

    template<typename T> void
    getinfo(CURLINFO info, T *t) {
        check(curl_easy_getinfo(curl_, info, t));
    }
    
    void appendHeaders(const std::vector<std::string> &headers, time_t modified_since) {

        bool expect = false;
        bool connection = false;
        bool if_modified_since = false;
        for(std::vector<std::string>::const_iterator it = headers.begin();
            it != headers.end();
            ++it) {
            headers_helper_.append(it->c_str());
            if (!expect &&
                strncasecmp(it->c_str(), "Expect:", sizeof("Expect:") - 1) == 0) {
                expect = true;
            }
            else if (!connection &&
                     strncasecmp(it->c_str(), "Connection:", sizeof("Connection:") - 1) == 0) {
                connection = true;
            }
            else if (!if_modified_since &&
                     strncasecmp(it->c_str(), "If-Modified-Since:", sizeof("If-Modified-Since:") - 1) == 0) {
                if_modified_since = true;
            }
        }

        if (!expect) {
            headers_helper_.append("Expect:");
        }
        
        if (!connection) {
            headers_helper_.append("Connection: close");
        }
        
        if (!if_modified_since) {
            if (modified_since != Tag::UNDEFINED_TIME) {
                std::string header = "If-Modified-Since: ";
                header += HttpDateUtils::format(modified_since);
                headers_helper_.append(header.c_str());
                sent_modified_since_ = true;
            }
        }
        else {
            sent_modified_since_ = true;
        }

        setopt(CURLOPT_HTTPHEADER, headers_helper_.get());
    }
    
    void postData(const void* data, long size) {
        setopt(CURLOPT_HTTPPOST, static_cast<long>(1));
        setopt(CURLOPT_POSTFIELDS, data);
        setopt(CURLOPT_POSTFIELDSIZE, size);
    }
    
    long perform() {
        check(curl_easy_perform(curl_));
        getinfo(CURLINFO_RESPONSE_CODE, &status_);
        detectContentType();
        return status_;
    }
    
    void detectContentType() {
        std::multimap<std::string, std::string>::const_iterator i = headers_.find("content-type");
        if (headers_.end() != i) {
            const std::string &type = i->second;
            typedef boost::char_separator<char> Separator;
            typedef boost::tokenizer<Separator> Tokenizer;

            Tokenizer tok(type, Separator(" ;"));
            for (Tokenizer::iterator i = tok.begin(), end = tok.end(); i != end; ++i) {
                if (content_type_.empty()) {
                    std::string::size_type pos = i->find_first_of(',');
                    if (pos == std::string::npos) {
                        content_type_.assign(*i);
                    }
                    else {
                        content_type_.assign(i->begin(), i->begin() + pos);
                    }
                }
                else if (i->find("charset=") == 0) {
                    charset_.assign(i->substr(sizeof("charset=") - 1));
                }
            }
            log()->debug("found %s, %s", content_type_.c_str(), charset_.c_str());
        }
        else {
            charset_.assign("windows-1251");
            content_type_.assign("text/xml");
        }
    }
    
    void check(CURLcode code) const {
        if (CURLE_OK != code) {
            throw std::runtime_error(curl_easy_strerror(code));
        }
    }
    
    static size_t curlWrite(void *ptr, size_t size, size_t nmemb, void *arg) {
        try {
            std::string *str = static_cast<std::string*>(arg);
            str->append((const char*) ptr, size * nmemb);
            return (size * nmemb);
        }
        catch(const std::exception &e) {
            log()->error("caught exception while curl write result: %s", e.what());
        }
        catch(...) {
            log()->error("caught unknown exception while curl write result");
        }
        return 0;
    }

    static size_t curlHeaders(void *ptr, size_t size, size_t nmemb, void *arg) {
        try {
            typedef std::multimap<std::string, std::string> HeaderMap;
            HeaderMap *m = static_cast<HeaderMap*>(arg);
            const char *header = (const char*) ptr, *pos = strchr(header, ':');
            if (NULL != pos) {
        
                std::pair<std::string, std::string> pair;
                Range name = trim(Range(header, pos)), value = trim(createRange(pos + 1));
        
                pair.first.reserve(name.size());
                std::transform(name.begin(), name.end(), std::back_inserter(pair.first), &tolower);
        
                pair.second.reserve(value.size());
                pair.second.assign(value.begin(), value.end());
        
                m->insert(pair);
            }
            return (size * nmemb);
        }
        catch(const std::exception &e) {
            log()->error("caught exception while curl process header: %s", e.what());
        }
        catch(...) {
            log()->error("caught unknown exception while curl process header");
        }
        return 0;
    }
    
    static void initEnvironment() {
        if (atexit(&destroyEnvironment) != 0) {
            std::stringstream stream;
            StringUtils::report("curl init failed: ", errno, stream);
            throw std::runtime_error(stream.str());
        }
        if (CURLE_OK != curl_global_init(CURL_GLOBAL_ALL)) {
            throw std::runtime_error("libcurl init failed");
        }
    }

    static void destroyEnvironment() {
        curl_global_cleanup();
    }
    
    HeadersHelper headers_helper_;
    CURL *curl_;
    long status_;
    std::multimap<std::string, std::string> headers_;
    std::string url_, charset_, content_type_;
    boost::shared_ptr<std::string> content_;
    bool sent_modified_since_;

    static boost::once_flag init_flag_;
};

boost::once_flag HttpHelper::HelperData::init_flag_ = BOOST_ONCE_INIT;


HttpHelper::HttpHelper(const std::string &url, long timeout) :
    data_(new HelperData(url, timeout))
{}

HttpHelper::~HttpHelper() {
    delete data_;
}

void
HttpHelper::init() {
    boost::call_once(&initEnvironment, HelperData::init_flag_);
}

void
HttpHelper::appendHeaders(const std::vector<std::string> &headers, time_t modified_since) {
    data_->appendHeaders(headers, modified_since);    
}

void
HttpHelper::postData(const void* data, long size) {
    data_->postData(data, size);
}

long
HttpHelper::perform() {
    log()->debug("%s", BOOST_CURRENT_FUNCTION);
    return data_->perform();
}

long
HttpHelper::status() const {
    return data_->status_;
}

const std::string&
HttpHelper::url() const {
    return data_->url_;
}

const std::string&
HttpHelper::charset() const {
    return data_->charset_;
}

boost::shared_ptr<std::string>
HttpHelper::content() const {
    return data_->content_;
}

const std::string&
HttpHelper::contentType() const {
    return data_->content_type_;
}

const std::multimap<std::string, std::string>&
HttpHelper::headers() const {
    return data_->headers_;
}

std::string
HttpHelper::base() const {
    std::string::size_type pos = data_->url_.find('?');
    if (std::string::npos != pos) {
        return data_->url_.substr(0, pos);
    }
    return data_->url_;
}

void
HttpHelper::checkStatus() const {
    log()->debug("%s, status: %ld", BOOST_CURRENT_FUNCTION, data_->status_);
    if (data_->status_ >= 400) {
        std::stringstream stream;
        stream << "server responded " << data_->status_;
        throw std::runtime_error(stream.str());
    }
    else if (0 == data_->status_ && 0 == data_->content_->size()) {
        throw std::runtime_error("empty local content: possibly not performed");
    }
    else if (304 == data_->status_ && !data_->sent_modified_since_) {
        throw std::runtime_error("server responded not-modified but if-modified-since was not sent");
    }
}

Tag
HttpHelper::createTag() const {
    Tag tag;
    if (304 == data_->status_) {
        tag.modified = false;
    }
    else if (200 == data_->status_ || 0 == data_->status_) {
        std::multimap<std::string, std::string>::const_iterator im = data_->headers_.find("last-modified");

        if (im != data_->headers_.end()) {
            tag.last_modified = HttpDateUtils::parse(im->second.c_str());
            log()->debug("%s, last_modified: %llu", BOOST_CURRENT_FUNCTION,
                static_cast<unsigned long long>(tag.last_modified));
        }
        std::multimap<std::string, std::string>::const_iterator ie = data_->headers_.find("expires");
        if (ie != data_->headers_.end()) {
            tag.expire_time = HttpDateUtils::parse(ie->second.c_str());
            log()->debug("%s, expire_time: %llu", BOOST_CURRENT_FUNCTION,
                static_cast<unsigned long long>(tag.expire_time));
        }
    }

    return tag;
}

void
HttpHelper::detectContentType() {
    data_->detectContentType();
}

void
HttpHelper::initEnvironment() {
    HelperData::initEnvironment();
}

void
HttpHelper::destroyEnvironment() {
    HelperData::destroyEnvironment();
}

bool
HttpHelper::isXml() const {
    const std::string &content_type = contentType();
    std::string::size_type pos = content_type.find('/');
    if (pos == std::string::npos) {
        return false;
    }

    ++pos;
    if (0 == strncasecmp(content_type.c_str(), "text/", sizeof("text/") - 1)) {
        return 0 == strcasecmp(content_type.c_str() + pos, "xml") ||
               0 == strcasecmp(content_type.c_str() + pos, "xml-external-parsed-entity");
    }
    else if (0 == strncasecmp(content_type.c_str(), "application/", sizeof("application/") - 1)) {
        std::string::size_type pos_plus = content_type.rfind('+');
        if (pos_plus == std::string::npos) {
            return 0 == strcasecmp(content_type.c_str() + pos, "xml") ||
                   0 == strcasecmp(content_type.c_str() + pos, "xml-external-parsed-entity") ||
                   0 == strcasecmp(content_type.c_str() + pos, "xml-dtd");
        }
        ++pos_plus;
        return 0 == strcasecmp(content_type.c_str() + pos_plus, "xml");
    }
    
    return false;
}

} // namespace xscript
