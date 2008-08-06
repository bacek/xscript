#include "settings.h"

#include <limits>

#include "xscript/tag.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

const time_t Tag::UNDEFINED_TIME = 0;

class TagPrefetchCalculator {
public:
    static bool needPrefetch(const Tag& tag, time_t stored_time);

private:
    static const time_t MINIMAL_PREFETCH_TIME = 1;
    static const time_t MAXIMAL_PREFETCH_TIME = 5;
    static const time_t LIMIT_CACHE_TIME = 60;
    static const time_t CACHE_TIME_PER_SECOND = LIMIT_CACHE_TIME / (MAXIMAL_PREFETCH_TIME - MINIMAL_PREFETCH_TIME);
};

bool TagPrefetchCalculator::needPrefetch(const Tag& tag, time_t stored_time) {

    if (Tag::UNDEFINED_TIME == tag.expire_time) {
        return false;
    }

    const time_t now = time(NULL);
    if (now >= tag.expire_time) {
        return true;
    }

    const time_t left_time = tag.expire_time - now;
    if (stored_time == Tag::UNDEFINED_TIME) {
        if (Tag::UNDEFINED_TIME == tag.last_modified) {
            return left_time <= MINIMAL_PREFETCH_TIME;
        }
        stored_time = tag.last_modified;
    }

    const time_t cache_time = tag.expire_time - stored_time;
    if (LIMIT_CACHE_TIME <= cache_time) {
        return left_time <= MAXIMAL_PREFETCH_TIME;
    }

    const time_t limit_prefetch_time = MINIMAL_PREFETCH_TIME + cache_time / CACHE_TIME_PER_SECOND;
    return left_time <= limit_prefetch_time;
}


Tag::Tag() :
        modified(true), last_modified(UNDEFINED_TIME), expire_time(UNDEFINED_TIME) {
}

Tag::Tag(bool mod, time_t last_mod, time_t exp_time) :
        modified(mod), last_modified(last_mod), expire_time(exp_time) {
}

bool
Tag::expired() const {
    return (UNDEFINED_TIME != expire_time) && (expire_time <= time(NULL));
}

bool
Tag::needPrefetch(time_t stored_time) const {
    return TagPrefetchCalculator::needPrefetch(*this, stored_time);
}


} // namespace xscript
