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
	const std::string& val = value();
	if (val == "path"){
		const std::string& script_name = req->getScriptName();
		const std::string& query_string = req->getQueryString();
		if (query_string.empty()) {
			return script_name;
		}
		else {
			return script_name + "?" + query_string;
		}
	}
	else if (val == "pathinfo") {
		return req->getPathInfo();
	}
	else if (val == "realpath") {
		return req->getPathTranslated();
	}
	else if (val == "originalpath") {
		std::string name = "X-Original-Path";
		if (req->hasHeader(name)) {
			return req->getHeader(name);
		}
		else {
			const std::string& query_string = req->getQueryString();
			if (query_string.empty()) {
				return req->getScriptName();
			}
			else {
				return req->getScriptName() + "?" + query_string;
			}
		}	
	}
	else if (val == "query") {
		return req->getQueryString();
	}
	else if (val == "remote_ip") {
		return req->getRealIP();
	}
	else if (val == "uri") {
		return req->getScriptName();
	}
	else if (val == "method") {
		return req->getRequestMethod();
	}
	else if (val == "secure") {
		return req->isSecure() ? "yes" : "no";
	}
	else if (val == "user") {
		return req->getRemoteUser();
	}
	else if (val == "content-length") {
		return boost::lexical_cast<std::string>(req->getContentLength());
	}
	else if (val == "content-encoding") {
		return req->getContentEncoding();
	}
	else if (val == "content-type") {
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
