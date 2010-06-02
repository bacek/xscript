#ifndef _XSCRIPT_HTTP_HELPER_H_
#define _XSCRIPT_HTTP_HELPER_H_

#include <string>
#include <time.h>

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include "xscript/tag.h"


namespace xscript {

class Request;

class HttpHelper : private boost::noncopyable {
public:
    HttpHelper(const std::string &url, long timeout);
    virtual ~HttpHelper();

    static void init();

    void appendHeaders(const std::vector<std::string> &headers, time_t modified_since);
    void postData(const void* data, long size);

    long perform();

    long status() const;
    const std::string& url() const;
    const std::string& charset() const;
    boost::shared_ptr<std::string> content() const;
    const std::string& contentType() const;
    const std::multimap<std::string, std::string>& headers() const;

    std::string base() const;
    void checkStatus() const;

    Tag createTag() const;

    bool isXml() const; 
protected:
    void detectContentType();

private:
    HttpHelper(const HttpHelper &);
    HttpHelper& operator = (const HttpHelper &);

    static void initEnvironment();
    static void destroyEnvironment();

private:
    class HelperData;
    std::auto_ptr<HelperData> data_;
};

} // namespace xscript

#endif // _XSCRIPT_HTTP_HELPER_H_
