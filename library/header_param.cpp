#include "settings.h"
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

    virtual std::string asString(const Context *ctx) const;

    static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
};

HeaderParam::HeaderParam(Object *owner, xmlNodePtr node) :
        TypedParam(owner, node) {
}

HeaderParam::~HeaderParam() {
}

std::string
HeaderParam::asString(const Context *ctx) const {
    Request *req = ctx->request();
    if (req->hasHeader(value())) {
        return req->getHeader(value());
    }
    return defaultValue();
}

std::auto_ptr<Param>
HeaderParam::create(Object *owner, xmlNodePtr node) {
    return std::auto_ptr<Param>(new HeaderParam(owner, node));
}

static CreatorRegisterer reg_("httpheader", &HeaderParam::create);

} // namespace xscript
