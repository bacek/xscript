#ifndef _XSCRIPT_REQUEST_RESPONSE_H_
#define _XSCRIPT_REQUEST_RESPONSE_H_

#include "xscript/request.h"
#include "xscript/response.h"

namespace xscript {

class RequestResponse : public Request, public Response {
public:
    RequestResponse();
    virtual ~RequestResponse();

    virtual bool suppressBody() const = 0;
};

} // namespace xscript

#endif // _XSCRIPT_REQUEST_RESPONSE_H_
