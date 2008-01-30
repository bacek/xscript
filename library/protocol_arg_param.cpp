#include "settings.h"

#include "xscript/param.h"
#include "xscript/request.h"
#include "xscript/context.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

class ProtocolArgParam : public TypedParam
{
public:
	ProtocolArgParam(Object *owner, xmlNodePtr node);
	virtual ~ProtocolArgParam();

	virtual std::string asString(const Context *ctx) const;

	static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
};

ProtocolArgParam::ProtocolArgParam(Object *owner, xmlNodePtr node) :
TypedParam(owner, node)
{
}

ProtocolArgParam::~ProtocolArgParam() {
}

std::string
ProtocolArgParam::asString(const Context *ctx) const {
	Request *req = ctx->request();
	if (value() == "http_path"){
		return req->getScriptName();
	}
	else if (value() == "http_path_info") {
		return req->getPathInfo();
	}
	else if (value() == "http_real_path") {
		return req->getPathTranslated();
	}
	else if (value() == "http_original_path") {
		std::string name = "X-Original-Path";
		if (req->hasHeader(name)) {
			return req->getHeader(name);
		}
		else {
			return req->getScriptName();
		}	
	}
	else if (value() == "http_query") {
		return req->getQueryString();
	}
	else if (value() == "http_remote_ip") {
		return req->getRemoteAddr();
	}

	return defaultValue();
}

std::auto_ptr<Param>
ProtocolArgParam::create(Object *owner, xmlNodePtr node) {
	return std::auto_ptr<Param>(new ProtocolArgParam(owner, node));
}

static CreatorRegisterer reg_("protocolarg", &ProtocolArgParam::create);

} // namespace xscript
