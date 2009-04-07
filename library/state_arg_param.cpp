#include "settings.h"

#include "xscript/context.h"
#include "xscript/param.h"
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
    State* state = ctx->state();
    if (state->has(value())) {
        return state->asString(value());
    }
    return defaultValue();
}

void
StateArgParam::add(const Context *ctx, ArgList &al) const {
    ConvertedParam::add(ctx, al);
}

std::auto_ptr<Param>
StateArgParam::create(Object *owner, xmlNodePtr node) {
    return std::auto_ptr<Param>(new StateArgParam(owner, node));
}

static CreatorRegisterer reg_("statearg", &StateArgParam::create);

} // namespace xscript
