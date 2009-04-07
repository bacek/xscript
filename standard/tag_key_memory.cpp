#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include "xscript/param.h"
#include "xscript/tagged_block.h"
#include "tag_key_memory.h"

namespace xscript {

TagKeyMemory::TagKeyMemory(const Context *ctx, const Object *obj) : value_() {
    assert(NULL != ctx);
    assert(NULL != obj);

    value_ = obj->createTagKey(ctx);
}

const std::string&
TagKeyMemory::asString() const {
    return value_;
}

}
