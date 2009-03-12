#include "settings.h"

#include "xscript/context.h"
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
    virtual std::string asString(const Context *ctx) const;

    static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
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

std::string
ProtocolArgParam::asString(const Context *ctx) const {
    std::string result = Protocol::get(ctx, value().c_str());
    return result.empty() ? defaultValue() : result;
}

std::auto_ptr<Param>
ProtocolArgParam::create(Object *owner, xmlNodePtr node) {
    return std::auto_ptr<Param>(new ProtocolArgParam(owner, node));
}

static CreatorRegisterer reg_("protocolarg", &ProtocolArgParam::create);

} // namespace xscript
