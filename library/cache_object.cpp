#include "settings.h"

#include <string.h>

#include "xscript/cache_object.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class CacheObject::ObjectData {
public:
    ObjectData();
    ~ObjectData();
    
    bool distributed_;
};

CacheObject::ObjectData::ObjectData() : distributed_(true)
{}

CacheObject::ObjectData::~ObjectData()
{}

CacheObject::CacheObject() : data_(new ObjectData())
{}

CacheObject::~CacheObject() {
    delete data_;
}

bool
CacheObject::checkProperty(const char *name, const char *value) {
    if (strncasecmp(name, "distributed-cache", sizeof("distributed-cache")) == 0) {
        data_->distributed_ = (strncasecmp(value, "no", sizeof("no")) != 0);
        return true;
    }
    return false;
}

bool
CacheObject::allowDistributed() const {
    return data_->distributed_;
}

} // namespace xscript
