#include "settings.h"

#include <cerrno>
#include <sstream>
#include <stdexcept>

#include <boost/lexical_cast.hpp>

#include "xscript/algorithm.h"
#include "xscript/http_utils.h"
#include "xscript/message_interface.h"
#include "xscript/string_utils.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

static const char httpDateUtilsShortWeekdays [7] [4] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
static const char httpDateUtilsShortMonths [12] [4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

const time_t HttpDateUtils::MAX_LIVE_TIME = std::numeric_limits<boost::int32_t>::max();

HttpDateUtils::HttpDateUtils() {
}

HttpDateUtils::~HttpDateUtils() {
}

std::string
HttpDateUtils::format(time_t value) {

    struct tm ts;
    memset(&ts, 0, sizeof(struct tm));

    if (NULL != gmtime_r(&value, &ts)) {
        char buf[255];

        //int res = strftime(buf, sizeof(buf), "%a, %d %b %Y %T GMT", &ts);
        int res = snprintf(buf, sizeof(buf) - 1, "%s, %02d %s %04d %02d:%02d:%02d GMT",
                           httpDateUtilsShortWeekdays[ts.tm_wday],
                           ts.tm_mday, httpDateUtilsShortMonths[ts.tm_mon], ts.tm_year + 1900,
                           ts.tm_hour, ts.tm_min, ts.tm_sec);

        if (0 < res) {
            return std::string(buf, buf + res);
        }
        else {
            std::stringstream stream;
            StringUtils::report("format date failed: ", errno, stream);
            throw std::runtime_error(stream.str());
        }
    }
    throw std::runtime_error("format date failed. Value: " + boost::lexical_cast<std::string>(value));
}

time_t
HttpDateUtils::parse(const char *value) {

    struct tm ts;
    memset(&ts, 0, sizeof(struct tm));

    const char *formats[] = { "%a, %d %b %Y %T GMT", "%A, %d-%b-%y %T GMT", "%a %b %d %T %Y" };
    for (unsigned int i = 0; i < sizeof(formats)/sizeof(const char*); ++i) {
        if (NULL != strptime(value, formats[i], &ts)) {
            return timegm(&ts); // mktime(&ts) - timezone;
        }
    }
    return static_cast<time_t>(0);
}

time_t
HttpDateUtils::expires(boost::uint32_t delta) {
    time_t now = time(NULL);
    if (0 == delta) {
        return now;
    }
    time_t max = MAX_LIVE_TIME;
    
    boost::uint64_t result = (boost::uint64_t)now + (boost::uint64_t)delta;
    
    if (result >= (boost::uint64_t)max) {
        return max;
    }
    
    return (time_t)result;
}

const std::string HttpUtils::NORMALIZE_HEADER_METHOD = "HTTP_UTILS_NORMALIZE_HEADER_METHOD";
const std::string HttpUtils::USER_AGENT_HEADER_NAME = "User-Agent";
const std::string HttpUtils::ACCEPT_HEADER_NAME = "Accept";
const std::string HttpUtils::IF_NONE_MATCH_HEADER_NAME = "If-None-Match";
const std::string HttpUtils::EXPIRES_HEADER_NAME = "Expires";
const std::string HttpUtils::ETAG_HEADER_NAME = "ETag";

HttpUtils::HttpUtils() {
}

HttpUtils::~HttpUtils() {
}

const char*
HttpUtils::statusToString(short status) {

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
HttpUtils::normalizeInputHeaderName(const Range &range) {

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
HttpUtils::normalizeOutputHeaderName(const std::string &name) {

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
HttpUtils::normalizeQuery(const Range &range) {
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
HttpUtils::normalizeHeader(const std::string &name, const Range &value, std::string &result) {   
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
HttpUtils::checkUrlEscaping(const Range &range) {
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
HttpUtils::checkHost(const Range &range) {
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

class NormalizeHeaderHandler : public MessageHandler {
    Result process(const MessageParams &params, MessageResultBase &result) {
        (void)params;
        result.set(false);
        return CONTINUE;
    }
};

class HttpUtilsHandlerRegisterer {
public:
    HttpUtilsHandlerRegisterer() {
        MessageProcessor::instance()->registerBack(HttpUtils::NORMALIZE_HEADER_METHOD,
            boost::shared_ptr<MessageHandler>(new NormalizeHeaderHandler()));
    }
};

static HttpUtilsHandlerRegisterer reg_http_utils_handlers;

} // namespace xscript
