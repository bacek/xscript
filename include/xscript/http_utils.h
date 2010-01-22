#ifndef _XSCRIPT_HTTP_UTILS_H_
#define _XSCRIPT_HTTP_UTILS_H_

#include <ctime>
#include <string>

#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>

#include "xscript/range.h"

namespace xscript {

class HttpDateUtils : private boost::noncopyable {
public:
    static time_t parse(const char *value);
    static std::string format(time_t value);
    static time_t expires(boost::uint32_t delta);
    
    static const time_t MAX_LIVE_TIME;
private:
    HttpDateUtils();
    virtual ~HttpDateUtils();
};

class HttpUtils : private boost::noncopyable  {
public:
    static bool normalizeHeader(const std::string &name, const Range &value, std::string &result);
    static std::string checkUrlEscaping(const Range &range);
    static Range checkHost(const Range &range);
    
    static std::string normalizeInputHeaderName(const Range &range);
    static std::string normalizeOutputHeaderName(const std::string &name);
    static std::string normalizeQuery(const Range &range);
    
    static const char* statusToString(short status);
    
    static const std::string NORMALIZE_HEADER_METHOD;
private:
    HttpUtils();
    virtual ~HttpUtils();
};

} // namespace xscript

#endif // _XSCRIPT_HTTP_UTILS_H_
