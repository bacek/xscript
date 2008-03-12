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

class ProductionCheckingPolicy : public CheckingPolicy
{
public:
	ProductionCheckingPolicy();
	~ProductionCheckingPolicy();
	void processError(const std::string& message);
	void sendError(Response* response, unsigned short status, const std::string& message);
	bool checkVariable(Request* request, const std::string& variable);
	bool isProduction() const;
};

class DevelopmentCheckingPolicy : public CheckingPolicy
{
public:
	DevelopmentCheckingPolicy();
	~DevelopmentCheckingPolicy();
	void processError(const std::string& message);
	void sendError(Response* response, unsigned short status, const std::string& message);
	bool checkVariable(Request* request, const std::string& variable);
	bool isProduction() const;
};

CheckingPolicy::CheckingPolicy() :
	use_profiler_(false)
{
}

CheckingPolicy::~CheckingPolicy() {
}

void
CheckingPolicy::init(const Config *config) {
	if (!isProduction()) {
		use_profiler_ = config->as<bool>("/xscript/xslt/use-profiler", false);
	}
}

void
CheckingPolicy::processError(const std::string& message) {
}

void
CheckingPolicy::sendError(Response* response, unsigned short status, const std::string& message) {
}

bool
CheckingPolicy::isProduction() const {
	return true;
}

bool
CheckingPolicy::useXSLTProfiler() const {
	return use_profiler_;
}

ProductionCheckingPolicy::ProductionCheckingPolicy() {
}

ProductionCheckingPolicy::~ProductionCheckingPolicy() {
}

void
ProductionCheckingPolicy::processError(const std::string& message) {
	log()->debug("%s", message.c_str());
}

void
ProductionCheckingPolicy::sendError(Response* response, unsigned short status, const std::string& message) {
	response->sendError(status, StringUtils::EMPTY_STRING);
}

bool
ProductionCheckingPolicy::isProduction() const {
	return true;
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
	}
	else {
		ComponentRegisterer<CheckingPolicy> reg(new ProductionCheckingPolicy());
	}

	CheckingPolicy::instance()->init(config);
}

} // namespace xscript
