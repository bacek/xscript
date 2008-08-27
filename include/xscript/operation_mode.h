#ifndef _XSCRIPT_CHECKING_POLICY_H_
#define _XSCRIPT_CHECKING_POLICY_H_

#include <string>
#include <xscript/component.h>

namespace xscript {

class Response;
class Request;
class Config;

class OperationMode : public Component<OperationMode> {
public:
    OperationMode();
    virtual ~OperationMode();
    virtual void processError(const std::string& message);
    virtual void sendError(Response* response, unsigned short status, const std::string& message);
    virtual bool isProduction() const;
};

/*
class DevelopmentCheckingPolicy : public CheckingPolicy {
public:
    DevelopmentCheckingPolicy();
    virtual ~DevelopmentCheckingPolicy();
    virtual void processError(const std::string& message);
    virtual void sendError(Response* response, unsigned short status, const std::string& message);
    virtual bool isProduction() const;
    virtual bool isOffline() const;
};
*/
} // namespace xscript

#endif //_XSCRIPT_CHECKING_POLICY_H_

