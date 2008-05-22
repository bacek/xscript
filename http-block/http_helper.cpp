#include "settings.h"

#include <cerrno>
#include <cstdlib>
#include <sstream>
#include <iterator>
#include <stdexcept>
#include <algorithm>

#include <boost/tokenizer.hpp>
#include <boost/current_function.hpp>

#include "http_helper.h"
#include "xscript/util.h"
#include "xscript/range.h"
#include "xscript/logger.h"
#include "xscript/request.h"
#include "internal/algorithm.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

class ProxyHeadersHelper
{
public:
	static bool skipped(const char* name);

private:
	struct StrSize
	{
		const char* name;
		int size;
	};

	static const StrSize skipped_headers[];
};


const ProxyHeadersHelper::StrSize ProxyHeadersHelper::skipped_headers[] = {
	{ "host", sizeof("host") },
	{ "if-modified-since", sizeof("if-modified-since") },
	{ "accept-encoding", sizeof("accept-encoding") },
	{ "keep-alive", sizeof("keep-alive") },
	{ "connection", sizeof("connection") },
};

bool
ProxyHeadersHelper::skipped(const char* name)
{
	for( int i = sizeof(skipped_headers) / sizeof(skipped_headers[0]); i--; ) {
		const StrSize& sh = skipped_headers[i];
		if(strncasecmp(name, sh.name, sh.size) == 0) {
			return true;
		}
	}
	return false;
}


HeadersHelper::HeadersHelper() : headers_(NULL)
{
}

HeadersHelper::~HeadersHelper() {
	clear();
}

const curl_slist*
HeadersHelper::get() const {
	return headers_;
}

void
HeadersHelper::append(const char *header) {
	headers_ = curl_slist_append(headers_, header);
	log()->debug("%s, added %s", BOOST_CURRENT_FUNCTION, header);
}

void
HeadersHelper::clear() {
	if (NULL != headers_) {
		curl_slist_free_all(headers_);
		headers_ = NULL;
	}
}


boost::once_flag HttpHelper::init_flag_ = BOOST_ONCE_INIT;

HttpHelper::HttpHelper(const std::string &url, long timeout) :
	curl_(NULL), status_(0), url_(url), sent_modified_since_(false)
{
	curl_ = curl_easy_init();
	if (NULL != curl_) {
		if (timeout > 0) {
			setopt(CURLOPT_TIMEOUT, (timeout + 999) / 1000);
		}
		setopt(CURLOPT_URL, url_.c_str());
		
		setopt(CURLOPT_NOSIGNAL, static_cast<long>(1));
		setopt(CURLOPT_NOPROGRESS, static_cast<long>(1));
		setopt(CURLOPT_FORBID_REUSE, static_cast<long>(1));
		
		setopt(CURLOPT_WRITEDATA, &content_);
		setopt(CURLOPT_WRITEFUNCTION, &curlWrite);
		setopt(CURLOPT_WRITEHEADER, &headers_);
		setopt(CURLOPT_HEADERFUNCTION, &curlHeaders);
	}
}

HttpHelper::~HttpHelper() {
	headers_helper_.clear();
	if (NULL != curl_) {
		curl_easy_cleanup(curl_);
	}
}

void
HttpHelper::init() {
	boost::call_once(&initEnvironment, init_flag_);
}

void
HttpHelper::appendHeaders(const Request* req, bool proxy, const Tag* tag) {
	log()->debug("%s, url:%s proxy:%d", BOOST_CURRENT_FUNCTION, url_.c_str(), proxy ? 1 : 0);

	if (NULL == req) {
		return;
	}

	std::string header;
	bool real_ip_added = false;
	if (proxy && req->countHeaders() > 0) {
		std::vector<std::string> names;
		req->headerNames(names);
		for (std::vector<std::string>::const_iterator i = names.begin(), end = names.end(); i != end; ++i) {
			const std::string& name = *i;
			const std::string& value = req->getHeader(name);
			if (ProxyHeadersHelper::skipped(name.c_str())) {
				log()->debug("%s, skipped %s: %s", BOOST_CURRENT_FUNCTION, name.c_str(), value.c_str());
			}
			else {
				header.assign(name).append(": ").append(value);
				headers_helper_.append(header.c_str());
				if (!real_ip_added) {
					real_ip_added = strncasecmp(name.c_str(), "x-real-ip", sizeof("x-real-ip")) == 0;
				}
			}
		}
	}
	if (!real_ip_added) {
		const std::string& remote_addr = req->getRemoteAddr();
		header.assign("X-Real-IP: ").append(remote_addr);
		headers_helper_.append(header.c_str());
	}

	headers_helper_.append("Expect:");
	headers_helper_.append("Connection: close");

	if (NULL != tag && tag->last_modified != Tag::UNDEFINED_TIME) {
		std::string header = "If-Modified-Since: ";
		header += HttpDateUtils::format(tag->last_modified);
		headers_helper_.append(header.c_str());
		sent_modified_since_ = true;
	}
	setopt(CURLOPT_HTTPHEADER, headers_helper_.get());
}

void
HttpHelper::postData(const void* data, long size) {
	setopt(CURLOPT_HTTPPOST, static_cast<long>(1));
	setopt(CURLOPT_POSTFIELDS, data);
	setopt(CURLOPT_POSTFIELDSIZE, size);
}

long
HttpHelper::perform() {
	log()->debug("%s", BOOST_CURRENT_FUNCTION);
	check(curl_easy_perform(curl_));
	getinfo(CURLINFO_RESPONSE_CODE, &status_);
	detectContentType();
	return status_;
}

long
HttpHelper::status() const {
	return status_;
}

const std::string&
HttpHelper::url() const {
	return url_;
}

const std::string&
HttpHelper::charset() const {
	return charset_;
}

const std::string&
HttpHelper::content() const {
	return content_;
}

const std::string&
HttpHelper::contentType() const {
	return content_type_;
}

const std::multimap<std::string, std::string>&
HttpHelper::headers() const {
	return headers_;
}
	
std::string
HttpHelper::base() const {
	std::string::size_type pos = url_.find('?');
	if (std::string::npos != pos) {
		return url_.substr(0, pos);
	}
	return url_;
}

void
HttpHelper::checkStatus() const {

	log()->debug("%s, status: %ld", BOOST_CURRENT_FUNCTION, status_);

	if (status_ >= 400) {
		std::stringstream stream;
		stream << "server responded " << status_;
		throw std::runtime_error(stream.str() + ". URL: " + url());
	}
	else if (0 == status_ && 0 == content_.size()) {
		throw std::runtime_error(
			std::string("empty local content: possibly not performed") + ". URL: " + url());
	}
	else if (304 == status_ && !sent_modified_since_) {
		throw std::runtime_error(
			std::string("server responded not-modified but if-modified-since was not sent") + ". URL: " + url());
	}
}

Tag
HttpHelper::createTag(boost::any &a) const {
	
	const Tag* tag = boost::any_cast<Tag>(&a);
	Tag result_tag;
	if (tag) {
		result_tag = *tag;
	}

	if (304 == status_) {
		result_tag.modified = false;
	}
	else if (200 == status_ || 0 == status_) {
		result_tag = Tag();
		std::multimap<std::string, std::string>::const_iterator im = headers_.find("last-modified");
		
		if (im != headers_.end()) {
			result_tag.last_modified = HttpDateUtils::parse(im->second.c_str());
			log()->debug("%s, last_modified: %lu", BOOST_CURRENT_FUNCTION, result_tag.last_modified);
		}
		std::multimap<std::string, std::string>::const_iterator ie = headers_.find("expires");
		if (ie != headers_.end()) {
			result_tag.expire_time = HttpDateUtils::parse(ie->second.c_str());
			log()->debug("%s, expire_time: %lu", BOOST_CURRENT_FUNCTION, result_tag.expire_time);
		}
	}

	return result_tag;
}

void
HttpHelper::detectContentType() {
	
	std::multimap<std::string, std::string>::const_iterator i = headers_.find("content-type");
	if (headers_.end() != i) {
		
		const std::string &type = i->second;
		typedef boost::char_separator<char> Separator;
		typedef boost::tokenizer<Separator> Tokenizer;
		
		Tokenizer tok(type, Separator(" ;"));
		for (Tokenizer::iterator i = tok.begin(), end = tok.end(); i != end; ++i) {
			if (content_type_.empty()) {
				content_type_.assign(*i);
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

void
HttpHelper::check(CURLcode code) const {
	if (CURLE_OK != code) {
		throw std::runtime_error(std::string(curl_easy_strerror(code)) + ". URL: " + url());
	}
}

size_t
HttpHelper::curlWrite(void *ptr, size_t size, size_t nmemb, void *arg) {
	std::string *str = static_cast<std::string*>(arg);
	str->append((const char*) ptr, size * nmemb);
	return (size * nmemb);
}

size_t
HttpHelper::curlHeaders(void *ptr, size_t size, size_t nmemb, void *arg) {
	
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

void
HttpHelper::initEnvironment() {
	try {
		if (atexit(&destroyEnvironment) != 0) {
			std::stringstream stream;
			StringUtils::report("curl init failed: ", errno, stream);
			throw std::runtime_error(stream.str());
		}
		if (CURLE_OK != curl_global_init(CURL_GLOBAL_ALL)) {
			throw std::runtime_error("libcurl init failed");
		}
	}
	catch (const std::exception &e) {
		log()->error("%s, caught exception: %s", BOOST_CURRENT_FUNCTION, e.what());
		throw;
	}
}

void
HttpHelper::destroyEnvironment() {
	curl_global_cleanup();
}

bool
HttpHelper::isXml() const {
	std::string::size_type pos = contentType().find("/");
	if (pos == std::string::npos) {
		return false;
	}

	std::string type = contentType().substr(0, pos);
	std::string subtype = contentType().substr(pos + 1);

	if (type == "text" && subtype == "xml") {
		return true;
	}

	if (type == "application") {
		if (subtype == "xml") {
			return true;
		}
		
		std::string::size_type pos_begin = subtype.rfind("+");
		if (pos_begin == std::string::npos) {
			pos_begin = 0;
		}
		else {
			pos_begin++;
		}
			
		if (subtype.substr(pos_begin) == "xml") {
			return true;
		}
	}

	return false;
}

} // namespace xscript
