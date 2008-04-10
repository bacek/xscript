#include <limits>
#include "details/dummy_tagged_cache.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

DummyTaggedCache::DummyTaggedCache()
{
}

DummyTaggedCache::~DummyTaggedCache() {
}

time_t
DummyTaggedCache::minimalCacheTime() const {
	return std::numeric_limits<time_t>::max();
}

bool
DummyTaggedCache::loadDocImpl(const TagKey *, Tag &, XmlDocHelper &) {
	return false;
}

bool
DummyTaggedCache::saveDocImpl(const TagKey *, const Tag &, const XmlDocHelper &) {
	return false;
}

std::auto_ptr<TagKey>
DummyTaggedCache::createKey(const Context*, const TaggedBlock *) const {
	return std::auto_ptr<TagKey>();
}


}
