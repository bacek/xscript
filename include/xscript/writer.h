#ifndef _XSCRIPT_WRITER_H_
#define _XSCRIPT_WRITER_H_

#include <string>
#include <boost/noncopyable.hpp>
#include <xscript/xml_helpers.h>

namespace xscript {

class Response;

class DocumentWriter : private boost::noncopyable {
public:
    DocumentWriter();
    virtual ~DocumentWriter();

    virtual const std::string& outputEncoding() const = 0;

    virtual void addHeaders(Response *response) = 0;
    virtual void write(Response *response, const XmlDocHelper &doc, xmlOutputBufferPtr buf) = 0;
};

class BinaryWriter {
public:
    virtual void write(std::ostream *os) const = 0;
    virtual std::streamsize size() const = 0;
};

} // namespace xscript

#endif // _XSCRIPT_WRITER_H_
