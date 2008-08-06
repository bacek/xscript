#include "settings.h"

#include <sstream>
#include <exception>

#include "xscript/util.h"
#include "xscript/cookie.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

const time_t Cookie::MAX_LIVE_TIME = std::numeric_limits<time_t>::max();

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

} // namespace xscript
