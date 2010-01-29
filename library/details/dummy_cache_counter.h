#ifndef _XSCRIPT_DETAILS_DUMMY_CACHE_COUNTER_H
#define _XSCRIPT_DETAILS_DUMMY_CACHE_COUNTER_H

#include "xscript/cache_counter.h"

namespace xscript {
/**
 * Do nothing counter
 */
class DummyCacheCounter : public CacheCounter {
public:
    void incUsedMemory(size_t amount);
    void decUsedMemory(size_t amount);

    void incLoaded();
    void incInserted();
    void incUpdated();
    void incExpired();
    void incExcluded();
    
    XmlNodeHelper createReport() const;
};


}

#endif



