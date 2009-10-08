#ifndef _XSCRIPT_SERVER_REQUEST_H_
#define _XSCRIPT_SERVER_REQUEST_H_

#include "xscript/response.h"

namespace xscript {

class ServerResponse : public Response {
public:
    ServerResponse(std::ostream *stream);
    virtual ~ServerResponse();

    void detach();

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
