#include "settings.h"

#include <boost/current_function.hpp>

#include "xscript/doc_cache.h"
#include "xscript/logger.h"
#include "xscript/tag.h"

#include "doc_pool.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

DocPool::DocPool(size_t size, const std::string &name) :
    counter_(CacheCounterFactory::instance()->createCounter(name))
{
    cache_ = std::auto_ptr<CacheType>(new CacheType(size, true, *counter_));
}

DocPool::~DocPool() {
    clear();
}

const CacheCounter*
DocPool::getCounter() const {
    return counter_.get();
}

bool
DocPool::loadDoc(const std::string &key, Tag &tag, boost::shared_ptr<CacheData> &cache_data,
        const CleanupFunc &cleanFunc) {
    log()->debug("%s, key: %s", BOOST_CURRENT_FUNCTION, key.c_str());
    return cache_->load(key, cache_data, tag, cleanFunc);
}

bool
DocPool::saveDoc(const std::string &key, const Tag &tag, const boost::shared_ptr<CacheData> &cache_data,
        const CleanupFunc &cleanFunc) {
    log()->debug("%s, key: %s", BOOST_CURRENT_FUNCTION, key.c_str());
    cache_->save(key, cache_data, tag, cleanFunc);
    return true;
}

void
DocPool::clear() {
    cache_->clear();
}

} // namespace xscript
