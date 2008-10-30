#ifndef _XSCRIPT_DOC_CACHE_H_
#define _XSCRIPT_DOC_CACHE_H_

#include <vector>
#include <string>
#include <xscript/component.h>
#include <xscript/control_extension.h>
#include <xscript/xml_helpers.h>

namespace xscript {

class Context;
class TaggedBlock;
class Tag;
class DocCacheStrategy;

class Block;
class Extension;
class Xml;

/**
 * Cache result of block invokations using sequence of different strategies.
 */
class DocCache : public Component<DocCache> {
public:
    DocCache();
    virtual ~DocCache();

    time_t minimalCacheTime() const;

    bool loadDoc(const Context *ctx, const TaggedBlock *block, Tag &tag, XmlDocHelper &doc);
    bool saveDoc(const Context *ctx, const TaggedBlock *block, const Tag& tag, const XmlDocHelper &doc);

    void init(const Config *config);
    void addStrategy(DocCacheStrategy* strategy, const std::string& name);

    /**
     * Create block to output statistic in xscript page.
     */
    std::auto_ptr<Block> createBlock(const ControlExtension *ext, Xml *owner, xmlNodePtr node);

    /**
     * Create aggregate report. Caller must free result.
     */
    XmlDocHelper createReport() const;

private:
    // Used for init added strategies
    const Config *config_;

    // Name of strategies in order of invokation
    std::vector<std::string> strategiesOrder_;

    // Sorted list of strategies
    std::vector<DocCacheStrategy*> strategies_;

};

} // namespace xscript

#endif // _XSCRIPT_DOC_CACHE_H_
