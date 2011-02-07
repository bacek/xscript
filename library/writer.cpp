#include "settings.h"

#include <sstream>
#include <libxml/xmlIO.h>
#include <libxml/xmlsave.h>
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
        encoding_(encoding), indent_(true) {
}

XmlWriter::XmlWriter(const std::string &encoding, bool indent) :
        encoding_(encoding), indent_(indent) {
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

int
XmlWriter::write(Response *response, xmlDocPtr doc, xmlOutputBufferPtr buf) {
    addHeaders(response);
    return xmlSaveFormatFileTo(buf, doc, encoding_.c_str(), indent_ ? 1 : 0);
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
        std::string::size_type pos = type.find(';');
        if (std::string::npos != pos) {
            type.erase(pos);
        }
        std::stringstream stream;
        stream << type << "; charset=" << stylesheet_->outputEncoding();
        response->setHeader("Content-type", stream.str());
    }
    else {
        response->setHeader("Content-type", type);
    }
}

int
HtmlWriter::write(Response *response, xmlDocPtr doc, xmlOutputBufferPtr buf) {
    addHeaders(response);
    int len = xsltSaveResultTo(buf, doc, stylesheet_->stylesheet());
    xmlOutputBufferClose(buf);
    return len;
}


XhtmlWriter::XhtmlWriter(const boost::shared_ptr<Stylesheet> &sh) :
        HtmlWriter(sh) {
}

XhtmlWriter::~XhtmlWriter() {
}

int
XhtmlWriter::write(Response *response, xmlDocPtr doc, xmlOutputBufferPtr buf) {
    addHeaders(response);

    int options = 16; // XML_SAVE_XHTML
    if (stylesheet_->indent()) {
        options |= XML_SAVE_FORMAT;
    }
    if (stylesheet_->omitXmlDecl()) {
        options |= XML_SAVE_NO_DECL;
    }

    xmlSaveCtxtPtr ctx = xmlSaveToIO(
        buf->writecallback, buf->closecallback, buf->context, outputEncoding().c_str(), options);
    xmlOutputBufferClose(buf);
    xmlSaveDoc(ctx, doc);
    return xmlSaveClose(ctx);
}


BinaryWriter::~BinaryWriter() {
}

} // namespace xscript
