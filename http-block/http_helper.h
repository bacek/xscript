#ifndef _XSCRIPT_HTTP_HELPER_H_
#define _XSCRIPT_HTTP_HELPER_H_

#include <map>
#include <string>
#include <time.h>
#include <boost/utility.hpp>
#include <boost/thread/once.hpp>
#include <curl/curl.h>

#include "xscript/tag.h"


namespace xscript
{

class HeadersHelper : private boost::noncopyable
{
public:
	HeadersHelper();
	~HeadersHelper();

	const curl_slist* get() const;

	void append(const char *header);
	void clear();

private:
	curl_slist *headers_;
};


class Request;

class HttpHelper : private boost::noncopyable
{
public:
	HttpHelper(const std::string &url, long timeout);
	virtual ~HttpHelper();

	static void init();

	void appendHeaders(const Request* req, bool proxy, const Tag* tag);
	void postData(const void* data, long size);

	long perform();
	template<typename T> void setopt(CURLoption opt, T t);
	template<typename T> void getinfo(CURLINFO info, T *t);

	long status() const;
	const std::string& url() const;
	const std::string& charset() const;
	const std::string& content() const;
	const std::string& contentType() const;
	const std::multimap<std::string, std::string>& headers() const;

	std::string base() const;
	void checkStatus() const;

	Tag createTag() const;

protected:
	void detectContentType();
	void check(CURLcode code) const;
	static size_t curlWrite(void *ptr, size_t size, size_t nmemb, void *arg);
	static size_t curlHeaders(void *ptr, size_t size, size_t nmemb, void *arg);
	
private:
	HttpHelper(const HttpHelper &);
	HttpHelper& operator = (const HttpHelper &);

	static void initEnvironment();
	static void destroyEnvironment();
	
private:
	HeadersHelper headers_helper_;
	CURL *curl_;
	long status_;
	std::multimap<std::string, std::string> headers_;
	std::string url_, charset_, content_, content_type_;
	bool sent_modified_since_;
	
	static boost::once_flag init_flag_;
};


template<typename T> void
HttpHelper::setopt(CURLoption opt, T t) {
	check(curl_easy_setopt(curl_, opt, t));
}

template<typename T> void
HttpHelper::getinfo(CURLINFO info, T *t) {
	check(curl_easy_getinfo(curl_, info, t));
}

} // namespace xscript

#endif // _XSCRIPT_HTTP_HELPER_H_
