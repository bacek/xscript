#include "settings.h"
#include "xscript/param.h"
#include "xscript/request.h"
#include "xscript/context.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

class CookieParam : public TypedParam
{
public:
	CookieParam(Object *owner, xmlNodePtr node);
	virtual ~CookieParam();
	
	virtual std::string asString(const Context *ctx) const;
	
	static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
};

CookieParam::CookieParam(Object *owner, xmlNodePtr node) :
	TypedParam(owner, node)
{
}

CookieParam::~CookieParam() {
}
	
std::string
CookieParam::asString(const Context *ctx) const {
	Request *req = ctx->request();
	if (req->hasCookie(value())) {
		return req->getCookie(value());
	}
	return defaultValue();
}

std::auto_ptr<Param>
CookieParam::create(Object *owner, xmlNodePtr node) {
	return std::auto_ptr<Param>(new CookieParam(owner, node));
}

static CreatorRegisterer reg_("cookie", &CookieParam::create);

} // namespace xscript
