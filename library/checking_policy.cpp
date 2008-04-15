#include "settings.h"
#include "xscript/checking_policy.h"
#include "xscript/config.h"
#include "xscript/component.h"
#include "xscript/response.h"
#include "xscript/logger.h"
#include "xscript/util.h"
#include "xscript/vhost_data.h"

#include <boost/lexical_cast.hpp>

#include <stdexcept>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

const std::string CheckingPolicyStarter::PRODUCTION_ID = "production";
const std::string CheckingPolicyStarter::DEVELOPMENT_ID = "development";

CheckingPolicy::CheckingPolicy() {
}

CheckingPolicy::~CheckingPolicy() {
}

void
CheckingPolicy::processError(const std::string& message) {
	log()->debug("%s", message.c_str());
}

void
CheckingPolicy::sendError(Response* response, unsigned short status, const std::string& message) {
	(void)message;
	response->sendError(status, StringUtils::EMPTY_STRING);
}

bool
CheckingPolicy::isProduction() const {
	return true;
}

bool
CheckingPolicy::useXSLTProfiler() const {
	return false;
}

DevelopmentCheckingPolicy::DevelopmentCheckingPolicy() {
}

DevelopmentCheckingPolicy::~DevelopmentCheckingPolicy() {
}

void
DevelopmentCheckingPolicy::processError(const std::string& message) {
	log()->error("%s", message.c_str());
	throw UnboundRuntimeError(message);
}

void
DevelopmentCheckingPolicy::sendError(Response* response, unsigned short status, const std::string& message) {
	response->sendError(status, message);
}

bool
DevelopmentCheckingPolicy::isProduction() const {
	return false;
}

bool
DevelopmentCheckingPolicy::useXSLTProfiler() const {
	return false;
}

CheckingPolicyStarter::CheckingPolicyStarter() 
{
}

CheckingPolicyStarter::~CheckingPolicyStarter() {
}

void
CheckingPolicyStarter::init(const Config *config) {
	std::string check_mode = config->as<std::string>("/xscript/check-mode", PRODUCTION_ID);
	if (DEVELOPMENT_ID == check_mode){
		ComponentRegisterer<CheckingPolicy> reg(new DevelopmentCheckingPolicy());
		(void)reg;
	}
	else {
		ComponentRegisterer<CheckingPolicy> reg(new CheckingPolicy());
		(void)reg;
	}
}

REGISTER_COMPONENT(CheckingPolicy);
REGISTER_COMPONENT(CheckingPolicyStarter);

} // namespace xscript
