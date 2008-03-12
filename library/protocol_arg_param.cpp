#include "settings.h"

#include "xscript/param.h"
#include "xscript/request.h"
#include "xscript/context.h"
#include "xscript/util.h"
#include "xscript/logger.h"

#include <boost/lexical_cast.hpp>

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
	if (value() == "path"){
		return req->getScriptName();
	}
	else if (value() == "pathinfo") {
		return req->getPathInfo();
	}
	else if (value() == "realpath") {
		return req->getPathTranslated();
	}
	else if (value() == "originalpath") {
		std::string name = "X-Original-Path";
		if (req->hasHeader(name)) {
			return req->getHeader(name);
		}
		else {
			return req->getScriptName();
		}	
	}
	else if (value() == "query") {
		return req->getQueryString();
	}
	else if (value() == "remote_ip") {
		return req->getRemoteAddr();
	}
	else if (value() == "uri") {
		const std::string& script_name = req->getScriptName();
		const std::string& query_string = req->getQueryString();
		if (query_string.empty()) {
			return script_name;
		}
		else {
			std::string uri = script_name + "?" + query_string;
			return uri;
		}
	}
	else if (value() == "method") {
		return req->getRequestMethod();
	}
	else if (value() == "secure") {
		return req->isSecure() ? "yes" : "no";
	}
	else if (value() == "user") {
		return req->getRemoteUser();
	}
	else if (value() == "content-length") {
		return boost::lexical_cast<std::string>(req->getContentLength());
	}
	else if (value() == "content-encoding") {
		if (req->hasHeader("content-encoding")) {
			return req->getHeader("content-encoding");
		}
		else {
			return StringUtils::EMPTY_STRING;
		}
	}
	else if (value() == "content-type") {
		return req->getContentType();
	}

	return defaultValue();
}

std::auto_ptr<Param>
ProtocolArgParam::create(Object *owner, xmlNodePtr node) {
	return std::auto_ptr<Param>(new ProtocolArgParam(owner, node));
}

static CreatorRegisterer reg_("protocolarg", &ProtocolArgParam::create);

} // namespace xscript
