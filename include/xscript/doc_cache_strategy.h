#ifndef _XSCRIPT_DOC_CACHE_STRATEGY_H_
#define _XSCRIPT_DOC_CACHE_STRATEGY_H_

#include <memory>
#include <vector>
#include <boost/utility.hpp>
#include <xscript/component.h>
#include <xscript/xml_helpers.h>
#include <xscript/stat_builder.h>
#include <xscript/average_counter.h>

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


class DocCacheStrategy
{
public:
	DocCacheStrategy();
	virtual ~DocCacheStrategy();

	virtual void init(const Config *config);

	virtual time_t minimalCacheTime() const = 0;
	virtual std::auto_ptr<TagKey> createKey(const Context *ctx, const TaggedBlock *block) const = 0;

	virtual bool loadDoc(const TagKey *key, Tag &tag, XmlDocHelper &doc);
	virtual bool saveDoc(const TagKey *key, const Tag& tag, const XmlDocHelper &doc);
	
protected:
	virtual bool loadDocImpl(const TagKey *key, Tag &tag, XmlDocHelper &doc) = 0;
	virtual bool saveDocImpl(const TagKey *key, const Tag& tag, const XmlDocHelper &doc) = 0;
	

	StatBuilder statBuilder_;
	AverageCounter hitCounter_;
	AverageCounter missCounter_;
	AverageCounter saveCounter_;
};

} // namespace xscript

#endif // _XSCRIPT_DOC_CACHE_STRATEGY_H_
