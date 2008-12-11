#include "settings.h"

#include <sstream>
#include <libxml/xmlIO.h>
#include <libxslt/xsltutils.h>

#include "xscript/writer.h"
#include "xscript/response.h"
#include "xscript/stylesheet.h"
#include "details/writer_impl.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

DocumentWriter::~DocumentWriter() {
}

XmlWriter::XmlWriter(const std::string &encoding) :
        encoding_(encoding) {
}

XmlWriter::~XmlWriter() {
}

const std::string&
XmlWriter::outputEncoding() const {
    return encoding_;
}

void
XmlWriter::addHeaders(Response *response) {
    std::string val = response->outputHeader("Content-type");
    if (!val.empty()) {
        return;
    }
    response->setHeader("Content-type", std::string("text/xml; charset=").append(encoding_));
}

void
XmlWriter::write(Response *response, const XmlDocHelper &doc, xmlOutputBufferPtr buf) {
    addHeaders(response);
    xmlSaveFormatFileTo(buf, doc.get(), encoding_.c_str(), 1);
}

HtmlWriter::HtmlWriter(const boost::shared_ptr<Stylesheet> &sh) :
        stylesheet_(sh) {
}

HtmlWriter::~HtmlWriter() {
}

const std::string&
HtmlWriter::outputEncoding() const {
    return stylesheet_->outputEncoding();
}

void
HtmlWriter::addHeaders(Response *response) {
    std::string val = response->outputHeader("Content-type");
    if (!val.empty()) {
        return;
    }
    std::string type = stylesheet_->contentType();
    if (type.find("text/") == 0) {
        std::stringstream stream;
        stream << type << "; charset=" << stylesheet_->outputEncoding();
        response->setHeader("Content-type", stream.str());
    }
    else {
        response->setHeader("Content-type", type);
    }
}

void
HtmlWriter::write(Response *response, const XmlDocHelper &doc, xmlOutputBufferPtr buf) {
    addHeaders(response);
    xsltSaveResultTo(buf, doc.get(), stylesheet_->stylesheet());
    xmlOutputBufferClose(buf);
}

BinaryWriter::~BinaryWriter() {
}

} // namespace xscript
