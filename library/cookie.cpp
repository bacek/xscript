#include "settings.h"

#include <sstream>
#include <exception>

#include "xscript/cookie.h"
#include "xscript/string_utils.h"
#include "xscript/util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

const time_t Cookie::MAX_LIVE_TIME = std::numeric_limits<boost::int32_t>::max();

Cookie::Cookie() {
}

Cookie::Cookie(const std::string &name, const std::string &value) :
        secure_(false), expires_(0),
        name_(name), value_(value), path_("/")

{
}

Cookie::~Cookie() {
}

std::string
Cookie::toString() const {

    if (!check()) {
        log()->warn("Incorrect cookie. Skipped. Name: %s; value: %s; domain: %s; path: %s",
                StringUtils::urlencode(name_).c_str(), StringUtils::urlencode(value_).c_str(),
                StringUtils::urlencode(domain_).c_str(), StringUtils::urlencode(path_).c_str());
        return StringUtils::EMPTY_STRING;
    }

    std::stringstream stream;
    stream << name_ << '=' << value_;
    if (!domain_.empty()) {
        stream << "; domain=" << domain_;
    }
    if (!path_.empty()) {
        stream << "; path=" << path_;
    }
    if (expires_) {
        stream <<  "; expires=" << HttpDateUtils::format(expires_);
    }
    if (secure_) {
        stream << "; secure";
    }
    return stream.str();
}

bool
Cookie::check() const {
     if (std::string::npos != name_.find_first_of("\r\n") ||
         std::string::npos != value_.find_first_of("\r\n") ||
         std::string::npos != domain_.find_first_of("\r\n") ||
         std::string::npos != path_.find_first_of("\r\n")) {
         return false;
     }
     return true;
}

} // namespace xscript
