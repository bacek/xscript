#ifndef _XSCRIPT_DOC_CACHE_H_
#define _XSCRIPT_DOC_CACHE_H_

#include <xscript/component.h>
#include <xscript/xml_helpers.h>

namespace xscript
{
	class Context;
	class TaggedBlock;
	class Tag;

	/**
	 * Cache result of block invokations using different strategies.
	 */
	class DocCache : public Component<DocCache>
	{
	public:
		DocCache();
		~DocCache();

		time_t minimalCacheTime() const;

		bool loadDoc(const Context *ctx, const TaggedBlock *block, Tag &tag, XmlDocHelper &doc);
		bool saveDoc(const Context *ctx, const TaggedBlock *block, const Tag& tag, const XmlDocHelper &doc);

	private:
	};
}

#endif // _XSCRIPT_DOC_CACHE_H_
