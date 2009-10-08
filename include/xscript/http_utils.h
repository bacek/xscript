#ifndef _XSCRIPT_HTTP_UTILS_H_
#define _XSCRIPT_HTTP_UTILS_H_

#include <string>

#include <boost/noncopyable.hpp>

#include "xscript/range.h"

namespace xscript {

class HttpUtils : private boost::noncopyable  {
public:
    static time_t parseDate(const char *value);
    static std::string formatDate(time_t value);
 
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
