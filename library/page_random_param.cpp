#include "settings.h"

#include "xscript/args.h"
#include "xscript/context.h"
#include "xscript/param.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class PageRandomParam : public ConvertedParam {
public:
    PageRandomParam(Object *owner, xmlNodePtr node);
    virtual ~PageRandomParam();

    virtual const char* type() const;
    virtual std::string asString(const Context *ctx) const;
    virtual void add(const Context *ctx, ArgList &al) const;

    static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
};

PageRandomParam::PageRandomParam(Object *owner, xmlNodePtr node) :
    ConvertedParam(owner, node) {
}

PageRandomParam::~PageRandomParam() {
}

void
PageRandomParam::add(const Context *ctx, ArgList &al) const {
    const std::string& as = ConvertedParam::as();
    as.empty() ? al.add(ctx->pageRandom()) : al.addAs(as, asString(ctx));
}

const char*
PageRandomParam::type() const {
    return "PageRandom";
}

std::string
PageRandomParam::asString(const Context *ctx) const {
    return boost::lexical_cast<std::string>(ctx->pageRandom());
}

std::auto_ptr<Param>
PageRandomParam::create(Object *owner, xmlNodePtr node) {
    return std::auto_ptr<Param>(new PageRandomParam(owner, node));
}

static CreatorRegisterer reg_("pagerandom", &PageRandomParam::create);

} // namespace xscript
