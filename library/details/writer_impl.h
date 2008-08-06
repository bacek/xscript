#ifndef _XSCRIPT_DETAILS_WRITER_IMPL_H_
#define _XSCRIPT_DETAILS_WRITER_IMPL_H_

#include "settings.h"

#include <boost/shared_ptr.hpp>

#include "xscript/writer.h"

namespace xscript {

class Stylesheet;

class XmlWriter : public DocumentWriter {
public:
    XmlWriter(const std::string &encoding);
    virtual ~XmlWriter();

    virtual const std::string& outputEncoding() const;

    virtual void addHeaders(Response *response);
    virtual void write(Response *response, const XmlDocHelper &doc, xmlOutputBufferPtr buf);

private:
    std::string encoding_;
};

class HtmlWriter : public DocumentWriter {
public:
    HtmlWriter(const boost::shared_ptr<Stylesheet> &sh);
    virtual ~HtmlWriter();

    virtual const std::string& outputEncoding() const;

    virtual void addHeaders(Response *response);
    virtual void write(Response *response, const XmlDocHelper &doc, xmlOutputBufferPtr buf);

private:
    boost::shared_ptr<Stylesheet> stylesheet_;
};

} // namespace xscript

#endif // _XSCRIPT_DETAILS_WRITER_IMPL_H_
