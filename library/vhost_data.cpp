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
VirtualHostData::set(Request* request) {
	request_.release();
	request_.reset(request);
}

Request*
VirtualHostData::get() const {
	return request_.get();
}

bool
VirtualHostData::hasVariable(const Request* request, const std::string& var) const {
	if (NULL == request) {
		request = request_.get();
		if (NULL == request) {
			return false;
		}
	}

	return request->hasVariable(var);
}

std::string
VirtualHostData::getVariable(const Request* request, const std::string& var) const {
	if (NULL == request) {
		request = request_.get();
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
