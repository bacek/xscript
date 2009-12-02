#include "settings.h"

#include "xscript/cache_object.h"

#include "tag_key_memory.h"

namespace xscript {

TagKeyMemory::TagKeyMemory(const Context *ctx, const CacheObject *obj) : value_() {
    assert(NULL != ctx);
    assert(NULL != obj);

    value_ = obj->createTagKey(ctx);
}

const std::string&
TagKeyMemory::asString() const {
    return value_;
}

}
