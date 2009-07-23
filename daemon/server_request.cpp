#include "settings.h"

#include <cctype>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <boost/lexical_cast.hpp>

#include "server_request.h"
#include "internal/parser.h"
#include "xscript/range.h"
#include "xscript/logger.h"
#include "xscript/encoder.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

ServerRequest::ServerRequest() : stream_(NULL) {
}

ServerRequest::~ServerRequest() {
}

void
ServerRequest::attach(std::istream *is, std::ostream *os, char *env[]) {
    try {
        stream_ = os;
        DefaultRequestResponse::attach(is, env);
        stream_->exceptions(std::ios::badbit);
    }
    catch(const BadRequestError &e) {
        throw;
    }
    catch(const std::exception &e) {
        throw BadRequestError(e.what());
    }
    catch(...) {
        throw BadRequestError("Unknown error");
    }
}

void
ServerRequest::detach() {
    DefaultRequestResponse::detach();
    (*stream_) << std::flush;
}

void
ServerRequest::writeError(unsigned short status, const std::string &message) {
    (*stream_) << "<html><body><h1>" << status << " " << Parser::statusToString(status) << "<br><br>"
    << XmlUtils::escape(createRange(message)) << "</h1></body></html>";
}

void
ServerRequest::writeBuffer(const char *buf, std::streamsize size) {
    stream_->write(buf, size);
}

void
ServerRequest::writeHeaders() {
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
ServerRequest::writeByWriter(const BinaryWriter *writer) {
    writer->write(stream_);
}

} // namespace xscript
