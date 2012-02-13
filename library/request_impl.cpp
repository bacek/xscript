#include "settings.h"

#include <string>

#include "internal/parser.h"
#include "internal/request_impl.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

const std::string RequestImpl::HEAD("HEAD");
const std::string RequestImpl::HOST_KEY("HOST");
const std::string RequestImpl::CONTENT_TYPE_KEY("Content-Type");
const std::string RequestImpl::CONTENT_ENCODING_KEY("Content-Encoding");
const std::string RequestImpl::CONTENT_LENGTH_KEY("Content-Length");

const std::string RequestImpl::HTTPS_KEY("HTTPS");
const std::string RequestImpl::SERVER_ADDR_KEY("SERVER_ADDR");
const std::string RequestImpl::SERVER_PORT_KEY("SERVER_PORT");

const std::string RequestImpl::PATH_INFO_KEY("PATH_INFO");
const std::string RequestImpl::PATH_TRANSLATED_KEY("PATH_TRANSLATED");

const std::string RequestImpl::SCRIPT_NAME_KEY("SCRIPT_NAME");
const std::string RequestImpl::SCRIPT_FILENAME_KEY("SCRIPT_FILENAME");
const std::string RequestImpl::DOCUMENT_ROOT_KEY("DOCUMENT_ROOT");

const std::string RequestImpl::REMOTE_USER_KEY("REMOTE_USER");
const std::string RequestImpl::REMOTE_ADDR_KEY("REMOTE_ADDR");

const std::string RequestImpl::QUERY_STRING_KEY("QUERY_STRING");
const std::string RequestImpl::REQUEST_METHOD_KEY("REQUEST_METHOD");
const std::string RequestImpl::REQUEST_URI_KEY("REQUEST_URI");

RequestImpl::RequestImpl() : id_(0), is_bot_(false)
{}

RequestImpl::~RequestImpl()
{}

void
RequestImpl::insertFile(
    const std::string &name, const std::map<Range, Range, RangeCILess> &m, const Range &content) {

    Range filename, type;

    std::map<Range, Range, RangeCILess>::const_iterator i = m.find(Parser::FILENAME_RANGE);
    if (m.end() == i) {
        throw std::runtime_error("uploaded file without name");
    }
    filename = i->second;
    if (filename.empty()) {
        return;
    }

    i = m.find(Parser::CONTENT_TYPE_MULTIPART_RANGE);
    if (m.end() != i) {
        type = i->second;
    }

    std::map<std::string, RequestFiles>::iterator it = files_.find(name);
    if (files_.end() == it) {
        it = files_.insert(make_pair(name, RequestFiles())).first;
    }

    RequestFile file(filename, type, content);
    it->second.push_back(file);
}

} // namespace xscript
