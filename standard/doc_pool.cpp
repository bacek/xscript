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

DocPool::DocPool(size_t size, const std::string &name) : cache_(size, true, name)
{}

DocPool::~DocPool() {
    clear();
}

const CacheCounter*
DocPool::getCounter() const {
    return cache_.counter();
}

bool
DocPool::loadDoc(const std::string &key, Tag &tag, boost::shared_ptr<CacheData> &cache_data) {
    log()->debug("%s, key: %s", BOOST_CURRENT_FUNCTION, key.c_str());
    return cache_.load(key, cache_data, tag);
}

bool
DocPool::saveDoc(const std::string &key, const Tag &tag, const boost::shared_ptr<CacheData> &cache_data) {
    log()->debug("%s, key: %s", BOOST_CURRENT_FUNCTION, key.c_str());
    cache_.save(key, cache_data, tag);
    return true;
}

void
DocPool::clear() {
    cache_.clear();
}

} // namespace xscript
