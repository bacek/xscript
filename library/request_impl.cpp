#include "settings.h"

#include <string>

#include "internal/parser.h"
#include "internal/request_impl.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

const std::string Request::RequestImpl::HEAD("HEAD");
const std::string Request::RequestImpl::HOST_KEY("HOST");
const std::string Request::RequestImpl::CONTENT_TYPE_KEY("Content-Type");
const std::string Request::RequestImpl::CONTENT_ENCODING_KEY("Content-Encoding");
const std::string Request::RequestImpl::CONTENT_LENGTH_KEY("Content-Length");

const std::string Request::RequestImpl::HTTPS_KEY("HTTPS");
const std::string Request::RequestImpl::SERVER_ADDR_KEY("SERVER_ADDR");
const std::string Request::RequestImpl::SERVER_PORT_KEY("SERVER_PORT");

const std::string Request::RequestImpl::PATH_INFO_KEY("PATH_INFO");
const std::string Request::RequestImpl::PATH_TRANSLATED_KEY("PATH_TRANSLATED");

const std::string Request::RequestImpl::SCRIPT_NAME_KEY("SCRIPT_NAME");
const std::string Request::RequestImpl::SCRIPT_FILENAME_KEY("SCRIPT_FILENAME");
const std::string Request::RequestImpl::DOCUMENT_ROOT_KEY("DOCUMENT_ROOT");

const std::string Request::RequestImpl::REMOTE_USER_KEY("REMOTE_USER");
const std::string Request::RequestImpl::REMOTE_ADDR_KEY("REMOTE_ADDR");

const std::string Request::RequestImpl::QUERY_STRING_KEY("QUERY_STRING");
const std::string Request::RequestImpl::REQUEST_METHOD_KEY("REQUEST_METHOD");

Request::RequestImpl::RequestImpl() : is_bot_(false)
{}

Request::RequestImpl::~RequestImpl()
{}

File::File(const std::map<Range, Range, RangeCILess> &m, const Range &content) :
        data_(content.begin(), static_cast<std::streamsize>(content.size())) {
    std::map<Range, Range, RangeCILess>::const_iterator i;
    i = m.find(Parser::CONTENT_TYPE_MULTIPART_RANGE);
    if (m.end() != i) {
        type_.assign(i->second.begin(), i->second.end());
    }
    i = m.find(Parser::FILENAME_RANGE);
    if (m.end() != i) {
        name_.assign(i->second.begin(), i->second.end());
    }
    else {
        throw std::runtime_error("uploaded file without name");
    }
}

const std::string&
File::type() const {
    return type_;
}

const std::string&
File::remoteName() const {
    return name_;
}

std::pair<const char*, std::streamsize>
File::data() const {
    return data_;
}

} // namespace xscript
