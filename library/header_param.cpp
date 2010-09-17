#include "settings.h"

#include "xscript/guard_checker.h"
#include "xscript/param.h"
#include "xscript/request.h"
#include "xscript/context.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class HeaderParam : public TypedParam {
public:
    HeaderParam(Object *owner, xmlNodePtr node);
    virtual ~HeaderParam();

    virtual const char* type() const;

    static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
    static bool is(const Context *ctx, const std::string &name, const std::string &value);

protected:
    virtual ValueResult getValue(const Context *ctx) const;
};

HeaderParam::HeaderParam(Object *owner, xmlNodePtr node) :
        TypedParam(owner, node) {
}

HeaderParam::~HeaderParam() {
}

const char*
HeaderParam::type() const {
    return "HttpHeader";
}

TypedParam::ValueResult
HeaderParam::getValue(const Context *ctx) const {
    if (NULL != ctx) {
        Request *req = ctx->request();
        if (req->hasHeader(value())) {
            return ValueResult(req->getHeader(value()), true);
        }
    }
    return ValueResult(StringUtils::EMPTY_STRING, false);
}

std::auto_ptr<Param>
HeaderParam::create(Object *owner, xmlNodePtr node) {
    return std::auto_ptr<Param>(new HeaderParam(owner, node));
}

bool
HeaderParam::is(const Context *ctx, const std::string &name, const std::string &value) {
    if (value.empty()) {
        return !ctx->request()->getHeader(name).empty();
    }
    return ctx->request()->getHeader(name) == value;
}

static CreatorRegisterer reg_("httpheader", &HeaderParam::create);
static GuardCheckerRegisterer reg2_("httpheader", &HeaderParam::is, false);

} // namespace xscript
