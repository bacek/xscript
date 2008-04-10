#ifndef _XSCRIPT_DETAILS_DUMMY_TAGGED_CACHE_H_
#define _XSCRIPT_DETAILS_DUMMY_TAGGED_CACHE_H_

#include "settings.h"
#include "xscript/tagged_cache.h"

namespace xscript
{

/**
 * Dummy TaggedCache. Don't cache at all.
 */
class DummyTaggedCache : public TaggedCache
{
public:
	DummyTaggedCache();
	virtual ~DummyTaggedCache();

	virtual time_t minimalCacheTime() const;
	virtual std::auto_ptr<TagKey> createKey(const Context *ctx, const TaggedBlock *block) const;
	
protected:
	virtual bool loadDocImpl(const TagKey *key, Tag &tag, XmlDocHelper &doc);
	virtual bool saveDocImpl(const TagKey *key, const Tag& tag, const XmlDocHelper &doc);
	
};


}

#endif
