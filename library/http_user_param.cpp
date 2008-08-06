#include "settings.h"

#include "xscript/args.h"
#include "xscript/param.h"
#include "xscript/request.h"
#include "xscript/context.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class HttpUserParam : public Param {
public:
    HttpUserParam(Object *owner, xmlNodePtr node);
    virtual ~HttpUserParam();

    virtual std::string asString(const Context *ctx) const;
    virtual void add(const Context *ctx, ArgList &al) const;

    static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);

};

HttpUserParam::HttpUserParam(Object *owner, xmlNodePtr node) :
        Param(owner, node) {
}

HttpUserParam::~HttpUserParam() {
}

std::string
HttpUserParam::asString(const Context *ctx) const {
    return ctx->request()->getRemoteUser();
}

void
HttpUserParam::add(const Context *ctx, ArgList &al) const {
    al.add(asString(ctx));
}

std::auto_ptr<Param>
HttpUserParam::create(Object *owner, xmlNodePtr node) {
    return std::auto_ptr<Param>(new HttpUserParam(owner, node));
}

static CreatorRegisterer reg_("httpuser", &HttpUserParam::create);

} // namespace xscript
