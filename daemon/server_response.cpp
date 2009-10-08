#include "settings.h"

#include <cctype>
#include <iostream>
#include <iterator>
#include <algorithm>

#include <boost/lexical_cast.hpp>

#include "xscript/encoder.h"
#include "xscript/exception.h"
#include "xscript/http_utils.h"
#include "xscript/logger.h"
#include "xscript/range.h"
#include "xscript/xml_util.h"
#include "xscript/writer.h"

#include "server_response.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

ServerResponse::ServerResponse(std::ostream *stream) : stream_(stream) {
    stream_->exceptions(std::ios::badbit);
}

ServerResponse::~ServerResponse() {
}

void
ServerResponse::detach() {
    Response::detach();
    (*stream_) << std::flush;
}

void
ServerResponse::writeError(unsigned short status, const std::string &message) {
    (*stream_) << "<html><body><h1>" << status << " " << HttpUtils::statusToString(status) << "<br><br>"
    << XmlUtils::escape(createRange(message)) << "</h1></body></html>";
}

void
ServerResponse::writeBuffer(const char *buf, std::streamsize size) {
    stream_->write(buf, size);
}

void
ServerResponse::writeHeaders() {
    const HeaderMap& headers = outHeaders();
    for (HeaderMap::const_iterator i = headers.begin(), end = headers.end(); i != end; ++i) {
        (*stream_) << i->first << ": " << i->second << "\r\n";
    }

    const CookieSet& cookies = outCookies();
    for (CookieSet::const_iterator i = cookies.begin(), end = cookies.end(); i != end; ++i) {
        (*stream_) << "Set-Cookie: " << i->toString() << "\r\n";
    }
    (*stream_) << "\r\n";
}

void
ServerResponse::writeByWriter(const BinaryWriter *writer) {
    writer->write(stream_);
}

} // namespace xscript
