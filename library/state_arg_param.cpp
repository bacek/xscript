#include "settings.h"

#include "xscript/context.h"
#include "xscript/guard_checker.h"
#include "xscript/param.h"
#include "xscript/string_utils.h"
#include "xscript/state.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class StateArgParam : public TypedParam {
public:
    StateArgParam(Object *owner, xmlNodePtr node);
    virtual ~StateArgParam();

    virtual const char* type() const;
    virtual std::string asString(const Context *ctx) const;
    
    virtual void add(const Context *ctx, ArgList &al) const;
    
    static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
    static bool is(const Context *ctx, const std::string &name, const std::string &value);
};

StateArgParam::StateArgParam(Object *owner, xmlNodePtr node) :
        TypedParam(owner, node) {
}

StateArgParam::~StateArgParam() {
}

const char*
StateArgParam::type() const {
    return "StateArg";
}

std::string
StateArgParam::asString(const Context *ctx) const {
    if (NULL == ctx)
        return "";
    else
        return ctx->state()->asString(value(), defaultValue());
}

void
StateArgParam::add(const Context *ctx, ArgList &al) const {
    ConvertedParam::add(ctx, al);
}

std::auto_ptr<Param>
StateArgParam::create(Object *owner, xmlNodePtr node) {
    return std::auto_ptr<Param>(new StateArgParam(owner, node));
}

bool
StateArgParam::is(const Context *ctx, const std::string &name, const std::string &value) {
    if (value.empty()) {
        return ctx->state()->is(name);
    }
    return ctx->state()->asString(name, StringUtils::EMPTY_STRING) == value;
}

static CreatorRegisterer reg_("statearg", &StateArgParam::create);
static GuardCheckerRegisterer reg2_("statearg", &StateArgParam::is, false);

} // namespace xscript
