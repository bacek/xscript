#ifndef _XSCRIPT_OPERATION_MODE_H_
#define _XSCRIPT_OPERATION_MODE_H_

#include <string>
#include <xscript/component.h>

namespace xscript {

class Response;

class OperationMode : public Component<OperationMode> {
public:
    OperationMode();
    virtual ~OperationMode();
    virtual void processError(const std::string& message);
    virtual void sendError(Response* response, unsigned short status, const std::string& message);
    virtual bool isProduction() const;
};

} // namespace xscript

#endif //_XSCRIPT_OPERATION_MODE_H_

