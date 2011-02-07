#ifndef _XSCRIPT_DETAILS_WRITER_IMPL_H_
#define _XSCRIPT_DETAILS_WRITER_IMPL_H_

#include <boost/shared_ptr.hpp>

#include "xscript/writer.h"

namespace xscript {

class Stylesheet;

class XmlWriter : public DocumentWriter {
public:
    explicit XmlWriter(const std::string &encoding);
    explicit XmlWriter(const std::string &encoding, bool indent);
    virtual ~XmlWriter();

    virtual const std::string& outputEncoding() const;

    virtual void addHeaders(Response *response);
    virtual int write(Response *response, xmlDocPtr doc, xmlOutputBufferPtr buf);

private:
    std::string encoding_;
    bool indent_;
};

class HtmlWriter : public DocumentWriter {
public:
    explicit HtmlWriter(const boost::shared_ptr<Stylesheet> &sh);
    virtual ~HtmlWriter();

    virtual const std::string& outputEncoding() const;

    virtual void addHeaders(Response *response);
    virtual int write(Response *response, xmlDocPtr doc, xmlOutputBufferPtr buf);

protected:
    boost::shared_ptr<Stylesheet> stylesheet_;
};

class XhtmlWriter : public HtmlWriter {
public:
    explicit XhtmlWriter(const boost::shared_ptr<Stylesheet> &sh);
    virtual ~XhtmlWriter();

    virtual int write(Response *response, xmlDocPtr doc, xmlOutputBufferPtr buf);
};

} // namespace xscript

#endif // _XSCRIPT_DETAILS_WRITER_IMPL_H_
