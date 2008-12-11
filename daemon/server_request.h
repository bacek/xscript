#ifndef _XSCRIPT_SERVER_REQUEST_H_
#define _XSCRIPT_SERVER_REQUEST_H_

#include <boost/cstdint.hpp>

#include "internal/default_request_response.h"

namespace xscript {

class ServerRequest : public DefaultRequestResponse {
public:
    ServerRequest();
    virtual ~ServerRequest();
    virtual void attach(std::istream *is, std::ostream *os, char *env[]);
    virtual void detach();

private:
    virtual void writeError(unsigned short status, const std::string &message);
    virtual void writeHeaders();
    virtual void writeBuffer(const char *buf, std::streamsize size);
    virtual void writeByWriter(const BinaryWriter *writer);

private:
    std::ostream *stream_;
};

} // namespace xscript

#endif // _XSCRIPT_SERVER_REQUEST_H_
