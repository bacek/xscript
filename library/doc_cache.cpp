#include "xscript/doc_cache.h"

namespace xscript
{

DocCache::DocCache() {
}

DocCache::~DocCache() {
}

time_t DocCache::minimalCacheTime() const {
	return 0;
}

bool DocCache::loadDoc(const Context *ctx, const TaggedBlock *block, Tag &tag, XmlDocHelper &doc) {
	return false;
}

bool DocCache::saveDoc(const Context *ctx, const TaggedBlock *block, const Tag& tag, const XmlDocHelper &doc) {
	return false;
}

REGISTER_COMPONENT(DocCache);

}
