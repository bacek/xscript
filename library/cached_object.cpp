#include "settings.h"

#include <string.h>
#include <stdexcept>

#include "xscript/cached_object.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class CachedObject::ObjectData {
public:
    ObjectData();
    ~ObjectData();
    
    Strategy strategy_;
};

CachedObject::ObjectData::ObjectData() : strategy_(SMART)
{}

CachedObject::ObjectData::~ObjectData()
{}

CachedObject::CachedObject() : data_(new ObjectData())
{}

CachedObject::~CachedObject() {
    delete data_;
}

bool
CachedObject::checkProperty(const char *name, const char *value) {
    if (strncasecmp(name, "distributed-cache", sizeof("distributed-cache")) == 0) {
        if (strncasecmp(value, "yes", sizeof("yes")) == 0) {
            data_->strategy_ = DISTRIBUTED;
        }
        else if (strncasecmp(value, "no", sizeof("no")) == 0) {
            data_->strategy_ = LOCAL;
        }
        else {
            throw std::runtime_error("incorrect distributed-cache value");
        }
        return true;
    }
    return false;
}

bool
CachedObject::allowDistributed() const {
    return data_->strategy_ != LOCAL;
}

CachedObject::Strategy
CachedObject::strategy() const {
    return data_->strategy_;
}

} // namespace xscript
