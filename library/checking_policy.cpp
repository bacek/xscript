#include "settings.h"
#include "xscript/checking_policy.h"
#include "xscript/config.h"
#include "xscript/component.h"
#include "xscript/response.h"
#include "xscript/logger.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

std::string CheckingPolicyStarter::PRODUCTION_ID = "production";
std::string CheckingPolicyStarter::DEVELOPMENT_ID = "development";

class ProductionCheckingPolicy : public CheckingPolicy
{
public:
	ProductionCheckingPolicy();
	~ProductionCheckingPolicy();
	void processError(const std::string& message);
	void sendError(Response* response, unsigned short status, const std::string& message);
};

class DevelopmentCheckingPolicy : public CheckingPolicy
{
public:
	DevelopmentCheckingPolicy();
	~DevelopmentCheckingPolicy();
	void processError(const std::string& message);
	void sendError(Response* response, unsigned short status, const std::string& message);
};

CheckingPolicy::CheckingPolicy() 
{
}

CheckingPolicy::~CheckingPolicy() {
}

void
CheckingPolicy::init(const Config *config) {
}

void
CheckingPolicy::processError(const std::string& message) {
}

void
CheckingPolicy::sendError(Response* response, unsigned short status, const std::string& message) {
}

ProductionCheckingPolicy::ProductionCheckingPolicy() {
}

ProductionCheckingPolicy::~ProductionCheckingPolicy() {
}

void
ProductionCheckingPolicy::processError(const std::string& message) {
	log()->debug("%s\n", message.c_str());
}

void
ProductionCheckingPolicy::sendError(Response* response, unsigned short status, const std::string& message) {
	response->sendError(status, "");
}

DevelopmentCheckingPolicy::DevelopmentCheckingPolicy() {
}

DevelopmentCheckingPolicy::~DevelopmentCheckingPolicy() {
}

void
DevelopmentCheckingPolicy::processError(const std::string& message) {
	log()->error("%s\n", message.c_str());
	throw std::runtime_error(message);
}

void
DevelopmentCheckingPolicy::sendError(Response* response, unsigned short status, const std::string& message) {
	response->sendError(status, message);
}

CheckingPolicyStarter::CheckingPolicyStarter() 
{
}

CheckingPolicyStarter::~CheckingPolicyStarter() {
}

void
CheckingPolicyStarter::init(const Config *config) {
	std::string check_mode = config->as<std::string>("/xscript/check_mode", PRODUCTION_ID);
	if (DEVELOPMENT_ID == check_mode){
		ComponentRegisterer<CheckingPolicy> reg(new DevelopmentCheckingPolicy());
	}
	else {
		ComponentRegisterer<CheckingPolicy> reg(new ProductionCheckingPolicy());
	}
}

} // namespace xscript