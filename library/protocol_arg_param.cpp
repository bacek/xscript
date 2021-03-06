#include "settings.h"

#include "xscript/context.h"
#include "xscript/exception.h"
#include "xscript/guard_checker.h"
#include "xscript/param.h"
#include "xscript/protocol.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class ProtocolArgParam : public TypedParam {
public:
    ProtocolArgParam(Object *owner, xmlNodePtr node);
    virtual ~ProtocolArgParam();

    virtual const char* type() const;

    static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
    static bool is(const Context *ctx, const std::string &name, const std::string &value);

protected:
    virtual ValueResult getValue(const Context *ctx) const;
};

ProtocolArgParam::ProtocolArgParam(Object *owner, xmlNodePtr node) :
        TypedParam(owner, node) {
}

ProtocolArgParam::~ProtocolArgParam() {
}

const char*
ProtocolArgParam::type() const {
    return "ProtocolArg";
}

TypedParam::ValueResult
ProtocolArgParam::getValue(const Context *ctx) const {
    try {
        std::string result = Protocol::get(ctx, value().c_str());
        return ValueResult(result, !result.empty());
    }
    catch(const std::runtime_error &e) {
        throw CriticalInvokeError(e.what());
    }
}

std::auto_ptr<Param>
ProtocolArgParam::create(Object *owner, xmlNodePtr node) {
    return std::auto_ptr<Param>(new ProtocolArgParam(owner, node));
}

bool
ProtocolArgParam::is(const Context *ctx, const std::string &name, const std::string &value) {
    try {
        if (value.empty()) {
            return !Protocol::get(ctx, name.c_str()).empty();
        }
        return Protocol::get(ctx, name.c_str()) == value;
    }
    catch(const std::runtime_error &e) {
        throw CriticalInvokeError(e.what());
    }
}

static CreatorRegisterer reg_("protocolarg", &ProtocolArgParam::create);
static GuardCheckerRegisterer reg2_("protocolarg", &ProtocolArgParam::is, false);

} // namespace xscript
