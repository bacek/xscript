#ifndef _XSCRIPT_WRITER_H_
#define _XSCRIPT_WRITER_H_

#include <string>
#include <boost/noncopyable.hpp>
#include <xscript/xml_helpers.h>

namespace xscript {

class Response;

class DocumentWriter : private boost::noncopyable {
public:
    virtual ~DocumentWriter();

    virtual const std::string& outputEncoding() const = 0;

    virtual void addHeaders(Response *response) = 0;
    virtual void write(Response *response, xmlDocPtr doc, xmlOutputBufferPtr buf) = 0;
};

class BinaryWriter : private boost::noncopyable {
public:
    virtual ~BinaryWriter();
	
    virtual void write(std::ostream *os, const Response *response) const = 0;
    virtual std::streamsize size() const = 0;
};

} // namespace xscript

#endif // _XSCRIPT_WRITER_H_
