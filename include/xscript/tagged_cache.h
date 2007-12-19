#ifndef _XSCRIPT_TAGGED_CACHE_H_
#define _XSCRIPT_TAGGED_CACHE_H_

#include <memory>
#include <vector>
#include <boost/utility.hpp>
#include <xscript/component.h>
#include <xscript/xml_helpers.h>

namespace xscript
{

class Tag;
class Context;
class TaggedBlock;

class TagKey : private boost::noncopyable
{
public:
	TagKey();
	virtual ~TagKey();
	virtual const std::string& asString() const = 0;
};

class TaggedCache : 
	public virtual Component,
	public ComponentHolder<TaggedCache>,
	public ComponentFactory<TaggedCache>
{
public:
	TaggedCache();
	virtual ~TaggedCache();

	virtual void init(const Config *config);
	
	virtual bool loadDoc(const TagKey *key, Tag &tag, XmlDocHelper &doc);
	virtual bool saveDoc(const TagKey *key, const Tag& tag, const XmlDocHelper &doc);
	
	virtual time_t minimalCacheTime() const;
	virtual std::auto_ptr<TagKey> createKey(const Context *ctx, const TaggedBlock *block) const;
};

} // namespace xscript

#endif // _XSCRIPT_TAGGED_CACHE_H_
