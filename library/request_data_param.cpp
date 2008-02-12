#include "settings.h"
#include "xscript/args.h"
#include "xscript/param.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

class RequestDataParam : public Param
{
public:
	RequestDataParam(Object *owner, xmlNodePtr node);
	virtual ~RequestDataParam();

	virtual std::string asString(const Context *ctx) const;
	virtual void add(const Context *ctx, ArgList &al) const;

	static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
};

RequestDataParam::RequestDataParam(Object *owner, xmlNodePtr node) :
Param(owner, node)
{
}

RequestDataParam::~RequestDataParam() {
}

std::string
RequestDataParam::asString(const Context *ctx) const {
	return std::string();
}

void
RequestDataParam::add(const Context *ctx, ArgList &al) const {
	al.addRequestData(ctx);
}

std::auto_ptr<Param>
RequestDataParam::create(Object *owner, xmlNodePtr node) {
	return std::auto_ptr<Param>(new RequestDataParam(owner, node));
}

static CreatorRegisterer reg_("requestdata", &RequestDataParam::create);

} // namespace xscript