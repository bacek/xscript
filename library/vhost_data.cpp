#include "settings.h"
#include "xscript/vhost_data.h"
#include "xscript/util.h"

#include "xscript/logger.h"

#include <boost/lexical_cast.hpp>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

VirtualHostData::VirtualHostData() 
{
}

VirtualHostData::~VirtualHostData() {
}

void
VirtualHostData::set(const Request* request) {
	request_provider_.reset(new RequestProvider(request));
}

const Request*
VirtualHostData::get() const {
	RequestProvider* provider = request_provider_.get();
	if (NULL == provider) {
		return NULL;
	}

	return provider->get();
}

bool
VirtualHostData::hasVariable(const Request* request, const std::string& var) const {
	if (NULL == request) {
		request = get();
		if (NULL == request) {
			return false;
		}
	}

	return request->hasVariable(var);
}

std::string
VirtualHostData::getVariable(const Request* request, const std::string& var) const {
	if (NULL == request) {
		request = get();
		if (NULL == request) {
			return StringUtils::EMPTY_STRING;
		}
	}

	return request->getVariable(var);
}

bool
VirtualHostData::checkVariable(Request* request, const std::string& var) const {

	if (hasVariable(request, var)) {
		std::string value = VirtualHostData::instance()->getVariable(request, var);
		if (strncasecmp("yes", value.c_str(), sizeof("yes") - 1) == 0 || 
			strncasecmp("true", value.c_str(), sizeof("true") - 1) == 0 ||
			boost::lexical_cast<bool>(value) == 1) {
				return true;
		}
	}

	return false;
}

std::string
VirtualHostData::getKey(const Request* request, const std::string& name) const {
	(void)request;
	return name;
}

std::string
VirtualHostData::getOutputEncoding(const Request* request) const {
	(void)request;
	return std::string("utf-8");
}

REGISTER_COMPONENT(VirtualHostData);

} // namespace xscript
