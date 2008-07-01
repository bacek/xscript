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
	std::string val = StringUtils::tolower(value());
	if (val == "path"){
		return req->getScriptName();
	}
	else if (val == "pathinfo") {
		return req->getPathInfo();
	}
	else if (val == "realpath") {
		return req->getScriptFilename();
	}
	else if (val == "originaluri") {
		return req->getOriginalURI();
	}
	else if (val == "originalurl") {
		return req->getOriginalUrl();
	}
	else if (val == "query") {
		return req->getQueryString();
	}
	else if (val == "remote_ip") {
		return req->getRealIP();
	}
	else if (val == "uri") {
		return req->getURI();
	}
	else if (val == "host") {
		return req->getHost();
	}
	else if (val == "originalhost") {
		return req->getOriginalHost();
	}
	else if (val == "method") {
		return req->getRequestMethod();
	}
	else if (val == "secure") {
		return req->isSecure() ? "yes" : "no";
	}
	else if (val == "http_user") {
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
