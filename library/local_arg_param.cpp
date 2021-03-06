#include "settings.h"

#include "xscript/context.h"
#include "xscript/guard_checker.h"
#include "xscript/param.h"
#include "xscript/string_utils.h"
#include "xscript/typed_map.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class LocalArgParam : public TypedParam {
public:
    LocalArgParam(Object *owner, xmlNodePtr node);
    virtual ~LocalArgParam();

    virtual const char* type() const;
    virtual std::string asString(const Context *ctx) const;
    
    virtual void add(const Context *ctx, ArgList &al) const;
    
    static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
    static bool is(const Context *ctx, const std::string &name, const std::string &value);
};

LocalArgParam::LocalArgParam(Object *owner, xmlNodePtr node) :
        TypedParam(owner, node) {
}

LocalArgParam::~LocalArgParam() {
}

const char*
LocalArgParam::type() const {
    return "LocalArg";
}

std::string
LocalArgParam::asString(const Context *ctx) const {
    return ctx ? ctx->getLocalParam(value(), defaultValue()) :
        StringUtils::EMPTY_STRING;
}

void
LocalArgParam::add(const Context *ctx, ArgList &al) const {
    addTypedValue(al, ctx->getLocalParam(value()), false); 
}

std::auto_ptr<Param>
LocalArgParam::create(Object *owner, xmlNodePtr node) {
    return std::auto_ptr<Param>(new LocalArgParam(owner, node));
}

bool
LocalArgParam::is(const Context *ctx, const std::string &name, const std::string &value) {
    if (value.empty()) {
        return ctx->localParamIs(name); 
    }
    return value == ctx->getLocalParam(name, StringUtils::EMPTY_STRING);
}

static CreatorRegisterer reg_("localarg", &LocalArgParam::create);
static GuardCheckerRegisterer reg2_("localarg", &LocalArgParam::is, false);

} // namespace xscript
