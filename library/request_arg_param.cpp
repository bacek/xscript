#include "settings.h"

#include "xscript/context.h"
#include "xscript/guard_checker.h"
#include "xscript/param.h"
#include "xscript/request.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class RequestArgParam : public TypedParam {
public:
    RequestArgParam(Object *owner, xmlNodePtr node);
    virtual ~RequestArgParam();

    virtual const char* type() const;

    static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
    static bool is(const Context *ctx, const std::string &name, const std::string &value);

protected:
    virtual ValueResult getValue(const Context *ctx) const;
};

RequestArgParam::RequestArgParam(Object *owner, xmlNodePtr node) : TypedParam(owner, node)
{
    Param::multi(true);
}

RequestArgParam::~RequestArgParam() {
}

const char*
RequestArgParam::type() const {
    return "RequestArg";
}

TypedParam::ValueResult
RequestArgParam::getValue(const Context *ctx) const {
    if (NULL != ctx) {
        const Request *req = ctx->request();
        if (req->hasArg(value())) {
            return ValueResult(req->getArg(value()), true);
        }
    }
    return ValueResult(StringUtils::EMPTY_STRING, false);
}

std::auto_ptr<Param>
RequestArgParam::create(Object *owner, xmlNodePtr node) {
    return std::auto_ptr<Param>(new RequestArgParam(owner, node));
}

bool
RequestArgParam::is(const Context *ctx, const std::string &name, const std::string &value) {
    const Request *req = ctx->request();
    if (value.empty()) {
        return !req->getArg(name).empty() || req->hasFile(name);
    }
    
    return req->getArg(name) == value;
}

static CreatorRegisterer reg_("requestarg", &RequestArgParam::create);
static GuardCheckerRegisterer reg2_("requestarg", &RequestArgParam::is, false);

} // namespace xscript
