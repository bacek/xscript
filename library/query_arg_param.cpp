#include "settings.h"

#include "xscript/context.h"
#include "xscript/guard_checker.h"
#include "xscript/param.h"
#include "xscript/request.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class QueryArgParam : public TypedParam {
public:
    QueryArgParam(Object *owner, xmlNodePtr node);
    virtual ~QueryArgParam();

    virtual const char* type() const;

    static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
    static bool is(const Context *ctx, const std::string &name, const std::string &value);

protected:
    virtual ValueResult getValue(const Context *ctx) const;
};

QueryArgParam::QueryArgParam(Object *owner, xmlNodePtr node) :
        TypedParam(owner, node) {
}

QueryArgParam::~QueryArgParam() {
}

const char*
QueryArgParam::type() const {
    return "QueryArg";
}

TypedParam::ValueResult
QueryArgParam::getValue(const Context *ctx) const {
    if (NULL != ctx) {
        Request *req = ctx->request();
        if (req->hasArg(value())) {
            return ValueResult(req->getArg(value()), true);
        }
    }
    return ValueResult(StringUtils::EMPTY_STRING, false);
}

std::auto_ptr<Param>
QueryArgParam::create(Object *owner, xmlNodePtr node) {
    return std::auto_ptr<Param>(new QueryArgParam(owner, node));
}

bool
QueryArgParam::is(const Context *ctx, const std::string &name, const std::string &value) {
    if (value.empty()) {
        return !ctx->request()->getArg(name).empty();
    }
    
    return ctx->request()->getArg(name) == value;
}

static CreatorRegisterer reg_("queryarg", &QueryArgParam::create);
static GuardCheckerRegisterer reg2_("queryarg", &QueryArgParam::is, false);

} // namespace xscript
