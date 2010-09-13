#include "settings.h"

#include <sstream>
#include <stdexcept>

#include "xscript/args.h"
#include "xscript/context.h"
#include "xscript/param.h"
#include "xscript/state.h"
#include "xscript/typed_map.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class StateParam : public Param {
public:
    StateParam(Object *owner, xmlNodePtr node);
    virtual ~StateParam();

    virtual const char* type() const;
    virtual std::string asString(const Context *ctx) const;
    virtual void add(const Context *ctx, ArgList &al) const;

    static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
};

StateParam::StateParam(Object *owner, xmlNodePtr node) :
        Param(owner, node) {
}

StateParam::~StateParam() {
}

const char*
StateParam::type() const {
    return "State";
}

std::string
StateParam::asString(const Context *ctx) const {
    return ctx->state()->asString();
}

void
StateParam::add(const Context *ctx, ArgList &al) const {
    al.addState(ctx);
}

std::auto_ptr<Param>
StateParam::create(Object *owner, xmlNodePtr node) {
    return std::auto_ptr<Param>(new StateParam(owner, node));
}

static CreatorRegisterer reg_("state", &StateParam::create);

} // namespace xscript
