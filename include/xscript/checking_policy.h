#ifndef _XSCRIPT_CHECKING_POLICY_H_
#define _XSCRIPT_CHECKING_POLICY_H_

#include <string>
#include <xscript/component.h>

namespace xscript
{

class Response;
class Config;

class CheckingPolicyStarter :
	public virtual Component, 
	public ComponentHolder<CheckingPolicyStarter>
{
public:
	CheckingPolicyStarter();
	virtual ~CheckingPolicyStarter();
	virtual void init(const Config *config);

	static const std::string PRODUCTION_ID;
	static const std::string DEVELOPMENT_ID;
};

class CheckingPolicy :
	public virtual Component, 
	public ComponentHolder<CheckingPolicy>
{
public:
	CheckingPolicy();
	virtual ~CheckingPolicy();
	virtual void init(const Config *config);
	virtual void processError(const std::string& message);
	virtual void sendError(Response* response, unsigned short status, const std::string& message);
};

}
#endif //_XSCRIPT_CHECKING_POLICY_H_

