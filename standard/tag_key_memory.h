#ifndef _XSCRIPT_STANDARD_TAG_KEY_MEMORY_H
#define _XSCRIPT_STANDARD_TAG_KEY_MEMORY_H

#include "xscript/doc_cache_strategy.h"

namespace xscript {

class TagKeyMemory : public TagKey {
public:
    TagKeyMemory(const Context *ctx, const TaggedBlock *block);
    virtual const std::string& asString() const;

private:
    std::string value_;
};

}

#endif

