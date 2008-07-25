#include <boost/current_function.hpp>
#include <boost/bind.hpp>
#include "xscript/logger.h"
#include "xscript/doc_cache.h"
#include "xscript/doc_cache_strategy.h"
#include "xscript/util.h"
#include "xscript/control_extension.h"

namespace xscript
{


class DocCacheBlock : public Block
{
public:
    DocCacheBlock(const Extension *ext, Xml *owner, xmlNodePtr node, const DocCache& doc_cache)
        : Block(ext, owner, node), doc_cache_(doc_cache)
    {
    }

    XmlDocHelper call(Context *, boost::any &) throw (std::exception) {
        return doc_cache_.createReport();
    }
private:
    const DocCache &doc_cache_;
};


DocCache::DocCache() {
}

DocCache::~DocCache() {
}

void DocCache::init(const Config *config) {
    // FIXME Read order of strategies
    config_ = config;

	ControlExtensionRegistry::constructor_t f = boost::bind(boost::mem_fn(&DocCache::createBlock), this, _1, _2, _3);

    ControlExtensionRegistry::registerConstructor("tagged-cache-stat", f);
}

void DocCache::addStrategy(DocCacheStrategy* strategy, const std::string& name) {
    // FIXME Add into proper position
    (void)name;
    log()->debug("%s %s", BOOST_CURRENT_FUNCTION, name.c_str());
    strategies_.push_back(strategy);
    strategy->init(config_);
}

time_t DocCache::minimalCacheTime() const {
    // FIXME.Do we actually need this in public part? Better to embed logick
    // of minimalCacheTime into saveDoc.
	return 0;
}

bool DocCache::loadDoc(const Context *ctx, const TaggedBlock *block, Tag &tag, XmlDocHelper &doc) {
    // FIXME Add saving of loaded doc into higher-order caches.
    log()->debug("%s", BOOST_CURRENT_FUNCTION);
    bool loaded = false;
    for(std::vector<DocCacheStrategy*>::iterator i = strategies_.begin();
        !loaded && i != strategies_.end();
        ++i) {
        std::auto_ptr<TagKey> key = (*i)->createKey(ctx, block);
        loaded = (*i)->loadDoc(key.get(), tag, doc);
    }
	return loaded;
}

bool DocCache::saveDoc(const Context *ctx, const TaggedBlock *block, const Tag& tag, const XmlDocHelper &doc) {
    bool saved = false;
    for(std::vector<DocCacheStrategy*>::iterator i = strategies_.begin();
        i != strategies_.end();
        ++i) {
        std::auto_ptr<TagKey> key = (*i)->createKey(ctx, block);
        saved |= (*i)->saveDoc(key.get(), tag, doc);
    }
	return saved;
}


std::auto_ptr<Block> DocCache::createBlock(const Extension *ext, Xml *owner, xmlNodePtr node) {
    return std::auto_ptr<Block>(new DocCacheBlock(ext, owner, node, *this));
}

XmlDocHelper DocCache::createReport() const {
    XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
	XmlUtils::throwUnless(NULL != doc.get());

	xmlNodePtr root = xmlNewDocNode(doc.get(), NULL, BAD_CAST "doc_cache", 0);
	XmlUtils::throwUnless(NULL != root);

	xmlDocSetRootElement(doc.get(), root);

        // Sorted list of strategies
    for (std::vector<DocCacheStrategy*>::const_iterator s = strategies_.begin();
        s != strategies_.end();
        ++s)
    {
		XmlDocHelper report = (*s)->createReport();
        XmlNodeHelper node(xmlCopyNode(xmlDocGetRootElement(report.get()), 1));
		xmlAddChild(root, node.get());
		node.release();
	}

	return doc;
}

REGISTER_COMPONENT(DocCache);

}
