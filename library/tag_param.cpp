#include "settings.h"

#include <stdexcept>
#include <boost/lexical_cast.hpp>

#include "details/tag_param.h"

#include "xscript/args.h"
#include "xscript/param.h"
#include "xscript/tagged_block.h"
#include "xscript/util.h"
#include "xscript/string_utils.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

TagParam::TagParam(TaggedBlock *owner, xmlNodePtr node) :
        Param(owner, node), owner_(owner) {
}

TagParam::~TagParam() {
}

const char*
TagParam::type() const {
    return "Tag";
}

std::string
TagParam::asString(const Context *ctx) const {
    (void)ctx;
    return StringUtils::EMPTY_STRING;
}

void
TagParam::add(const Context *ctx, ArgList &al) const {
    al.addTag(owner_, ctx);
}

std::auto_ptr<Param>
TagParam::create(Object *owner, xmlNodePtr node) {
    TaggedBlock* tblock = dynamic_cast<TaggedBlock*>(owner);
    if (NULL == tblock) {
        throw std::runtime_error("Conflict: tag param in non-tagged-block");
    }
    return std::auto_ptr<Param>(new TagParam(tblock, node));
}

static CreatorRegisterer reg_("tag", &TagParam::create);

} // namespace xscript
