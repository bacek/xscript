#include "settings.h"
#include "xscript/guard_checker.h"
#include "xscript/param.h"
#include "xscript/request.h"
#include "xscript/context.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class CookieParam : public TypedParam {
public:
    CookieParam(Object *owner, xmlNodePtr node);
    virtual ~CookieParam();

    virtual const char* type() const;

    static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
    static bool is(const Context *ctx, const std::string &name, const std::string &value);

protected:
    virtual ValueResult getValue(const Context *ctx) const;
};

CookieParam::CookieParam(Object *owner, xmlNodePtr node) :
        TypedParam(owner, node) {
}

CookieParam::~CookieParam() {
}

const char*
CookieParam::type() const {
    return "Cookie";
}

TypedParam::ValueResult
CookieParam::getValue(const Context *ctx) const {
    Request *req = ctx->request();
    if (req->hasCookie(value())) {
        return ValueResult(req->getCookie(value()), true);
    }
    return ValueResult(StringUtils::EMPTY_STRING, false);
}

std::auto_ptr<Param>
CookieParam::create(Object *owner, xmlNodePtr node) {
    return std::auto_ptr<Param>(new CookieParam(owner, node));
}

bool
CookieParam::is(const Context *ctx, const std::string &name, const std::string &value) {
    if (value.empty()) {
        return !ctx->request()->getCookie(name).empty();
    }
    return ctx->request()->getCookie(name) == value;
}

static CreatorRegisterer reg_("cookie", &CookieParam::create);
static GuardCheckerRegisterer reg2_("cookie", &CookieParam::is, false);

} // namespace xscript
