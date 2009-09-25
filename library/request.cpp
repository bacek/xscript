#include "settings.h"
#include "xscript/request.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

Request::Request() {
}

Request::~Request() {
}

boost::uint32_t
Request::getUContentLength() const {
    return static_cast<boost::uint32_t>(getContentLength());
}

} // namespace xscript
