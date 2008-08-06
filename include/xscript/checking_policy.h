#ifndef _XSCRIPT_CHECKING_POLICY_H_
#define _XSCRIPT_CHECKING_POLICY_H_

#include <string>
#include <xscript/component.h>

namespace xscript {

class Response;
class Request;
class Config;

class CheckingPolicyStarter : public Component<CheckingPolicyStarter> {
public:
    CheckingPolicyStarter();
    virtual ~CheckingPolicyStarter();
    virtual void init(const Config *config);

    static const std::string PRODUCTION_ID;
    static const std::string DEVELOPMENT_ID;
};

class CheckingPolicy : public Component<CheckingPolicy> {
public:
    CheckingPolicy();
    virtual ~CheckingPolicy();
    virtual void processError(const std::string& message);
    virtual void sendError(Response* response, unsigned short status, const std::string& message);
    virtual bool isProduction() const;
    virtual bool isOffline() const;
};

class DevelopmentCheckingPolicy : public CheckingPolicy {
public:
    DevelopmentCheckingPolicy();
    virtual ~DevelopmentCheckingPolicy();
    virtual void processError(const std::string& message);
    virtual void sendError(Response* response, unsigned short status, const std::string& message);
    virtual bool isProduction() const;
    virtual bool isOffline() const;
};

} // namespace xscript

#endif //_XSCRIPT_CHECKING_POLICY_H_

