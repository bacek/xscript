#include "settings.h"
#include "xscript/args.h"
#include "xscript/context.h"
#include "xscript/param.h"
#include "xscript/request.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class RequestParam : public Param {
public:
    RequestParam(Object *owner, xmlNodePtr node);
    virtual ~RequestParam();

    virtual const char* type() const;
    virtual std::string asString(const Context *ctx) const;
    virtual void add(const Context *ctx, ArgList &al) const;

    static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
};

RequestParam::RequestParam(Object *owner, xmlNodePtr node) :
        Param(owner, node) {
}

RequestParam::~RequestParam() {
}

const char*
RequestParam::type() const {
    return "Request";
}

std::string
RequestParam::asString(const Context *ctx) const {
    return ctx->request()->getQueryString();
}

void
RequestParam::add(const Context *ctx, ArgList &al) const {
    al.addRequest(ctx);
}

std::auto_ptr<Param>
RequestParam::create(Object *owner, xmlNodePtr node) {
    return std::auto_ptr<Param>(new RequestParam(owner, node));
}

static CreatorRegisterer reg_("request", &RequestParam::create);

} // namespace xscript

