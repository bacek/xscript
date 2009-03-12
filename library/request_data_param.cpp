#include "settings.h"
#include "xscript/args.h"
#include "xscript/context.h"
#include "xscript/param.h"
#include "xscript/request.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class RequestDataParam : public Param {
public:
    RequestDataParam(Object *owner, xmlNodePtr node);
    virtual ~RequestDataParam();

    virtual const char* type() const;
    virtual std::string asString(const Context *ctx) const;
    virtual void add(const Context *ctx, ArgList &al) const;

    static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
};

RequestDataParam::RequestDataParam(Object *owner, xmlNodePtr node) :
        Param(owner, node) {
}

RequestDataParam::~RequestDataParam() {
}

const char*
RequestDataParam::type() const {
    return "RequestData";
}

std::string
RequestDataParam::asString(const Context *ctx) const {
    return ctx->request()->getQueryString();
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

