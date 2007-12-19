#include "settings.h"

#include <limits>
#include "xscript/tagged_cache.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

TagKey::TagKey()
{
}

TagKey::~TagKey() {
}

TaggedCache::TaggedCache()
{
}

TaggedCache::~TaggedCache() {
}

void
TaggedCache::init(const Config *config) {
}

time_t
TaggedCache::minimalCacheTime() const {
	return std::numeric_limits<time_t>::max();
}

bool
TaggedCache::loadDoc(const TagKey *, Tag &, XmlDocHelper &) {
	return false;
}

bool
TaggedCache::saveDoc(const TagKey *, const Tag &, const XmlDocHelper &) {
	return false;
}

std::auto_ptr<TagKey>
TaggedCache::createKey(const Context*, const TaggedBlock *) const {
	return std::auto_ptr<TagKey>();
}

} // namespace xscript
