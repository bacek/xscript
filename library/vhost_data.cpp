#include "settings.h"
#include "xscript/vhost_data.h"
#include "xscript/util.h"

#include "xscript/logger.h"

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
VirtualHostData::init(const Config *config) {
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

std::string
VirtualHostData::getKey(const Request* request, const std::string& name) const {
	return name;
}

std::string
VirtualHostData::getOutputEncoding(const Request* request) const {
	return std::string("utf-8");
}

} // namespace xscript
