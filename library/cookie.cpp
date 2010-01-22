#include "settings.h"

#include <sstream>
#include <exception>

#include "xscript/cookie.h"
#include "xscript/http_utils.h"
#include "xscript/string_utils.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

Cookie::Cookie() {
}

Cookie::Cookie(const std::string &name, const std::string &value) :
        secure_(false), expires_(0),
        name_(name), value_(value), path_("/")

{
}

Cookie::~Cookie() {
}

time_t
Cookie::permanent() const {
    return expires_ == HttpDateUtils::MAX_LIVE_TIME;
}

void
Cookie::permanent(bool value) {
    expires_ = value ? HttpDateUtils::MAX_LIVE_TIME : 0;
}

std::string
Cookie::toString() const {

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
