#include "settings.h"

#include "internal/extension_list.h"

#include "xscript/algorithm.h"
#include "xscript/meta.h"
#include "xscript/meta_block.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

struct MetaCoreNotWritable {
    MetaCoreNotWritable(InvokeContext *invoke_ctx) : invoke_ctx_(invoke_ctx) {
        invoke_ctx_->meta()->coreWritable(false);
    }

    ~MetaCoreNotWritable() {
        invoke_ctx_->meta()->coreWritable(true);
    }
private:
    InvokeContext* invoke_ctx_;
};

MetaBlock::MetaBlock(const Block *block, xmlNodePtr node) :
        Block(block->extension(), block->owner(), node),
        parent_(block), root_ns_(node->ns), key_("meta")
{
    disableOutput(true);
}

MetaBlock::~MetaBlock() {
}

bool
MetaBlock::luaNode(const xmlNodePtr node) const {
    return node->ns && node->ns->href &&
           xmlStrcasecmp(node->name, (const xmlChar*)"lua") == 0 &&
           xmlStrcasecmp(node->ns->href, (const xmlChar*)XmlUtils::XSCRIPT_NAMESPACE) == 0;
}

bool
MetaBlock::luaAfterCacheLoadNode(const xmlNodePtr node) const {
    return node->ns && node->ns->href &&
           xmlStrcasecmp(node->name, (const xmlChar*)"lua-after-cache-load") == 0 &&
           xmlStrcasecmp(node->ns->href, (const xmlChar*)XmlUtils::XSCRIPT_NAMESPACE) == 0;
}

bool
MetaBlock::luaBeforeCacheSaveNode(const xmlNodePtr node) const {
    return node->ns && node->ns->href &&
           xmlStrcasecmp(node->name, (const xmlChar*)"lua-before-cache-save") == 0 &&
           xmlStrcasecmp(node->ns->href, (const xmlChar*)XmlUtils::XSCRIPT_NAMESPACE) == 0;
}

void
MetaBlock::parseSubNode(xmlNodePtr node) {
    if (metaNode(node)) {
        throw std::runtime_error("Nested meta section is not allowed");
    }
    if (luaNode(node)) {
        if (lua_block_.get()) {
            throw std::runtime_error("One common lua section allowed in meta");
        }
        parseLuaSection(node, lua_block_);
    }
    else if (luaAfterCacheLoadNode(node)) {
        if (after_cache_load_lua_.get()) {
            throw std::runtime_error("One after-cache-load lua section allowed in meta");
        }
        xmlNodeSetName(node, (const xmlChar*)"lua");
        parseLuaSection(node, after_cache_load_lua_);
    }
    else if (luaBeforeCacheSaveNode(node)) {
        if (before_cache_save_lua_.get()) {
            throw std::runtime_error("One before-cache-save lua section allowed in meta");
        }
        xmlNodeSetName(node, (const xmlChar*)"lua");
        parseLuaSection(node, before_cache_save_lua_);
    }
    else {
        Block::parseSubNode(node);
    }
}

void
MetaBlock::parseLuaSection(xmlNodePtr node, std::auto_ptr<Block> &block) {
    Extension *ext = ExtensionList::instance()->extension(node, false);
    if (NULL == ext) {
        throw std::runtime_error("Lua module is not loaded");
    }
    block = ext->createBlock(owner(), node);
    assert(block.get());
    block->parse();
}

void
MetaBlock::callLua(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) {
    if (lua_block_.get()) {
        boost::shared_ptr<InvokeContext> meta_ctx(new InvokeContext(invoke_ctx.get()));
        MetaCoreNotWritable holder(invoke_ctx.get());
        lua_block_->call(ctx, meta_ctx);
    }
}

void
MetaBlock::callBeforeCacheSaveLua(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) {
    if (before_cache_save_lua_.get()) {
        boost::shared_ptr<InvokeContext> meta_ctx(new InvokeContext(invoke_ctx.get()));
        before_cache_save_lua_->call(ctx, meta_ctx);
    }
}

void
MetaBlock::callAfterCacheLoadLua(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) {
    if (after_cache_load_lua_.get()) {
        boost::shared_ptr<InvokeContext> meta_ctx(new InvokeContext(invoke_ctx.get()));
        MetaCoreNotWritable holder(invoke_ctx.get());
        after_cache_load_lua_->call(ctx, meta_ctx);
    }
}

void
MetaBlock::call(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const throw (std::exception) {
    (void)ctx;
    XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != doc.get());

    XmlNodeHelper node(xmlNewNode(NULL, (const xmlChar*)root_name_.c_str()));
    XmlUtils::throwUnless(NULL != node.get());
    if (root_ns_) {
        node->nsDef = xmlCopyNamespace(root_ns_);
        xmlSetNs(node.get(), node->nsDef);
    }

    XmlNodeHelper meta_info = invoke_ctx->meta()->getXml();
    if (NULL != meta_info.get()) {
        xmlAddChildList(node.get(), meta_info.release());
    }

    xmlDocSetRootElement(doc.get(), node.release());
    invoke_ctx->metaDoc(doc);
}

void
MetaBlock::property(const char *name, const char *value) {
    if (strncasecmp(name, "name", sizeof("name")) == 0) {
        root_name_.assign(value);
        if (root_name_.empty()) {
            throw std::runtime_error("Empty name attribute is not allowed in meta");
        }
    }
    else {
        Block::property(name, value);
    }
}

void
MetaBlock::postParse() {

    addNamespaces(parent_->namespaces());

    if (!method().empty()) {
        throw std::runtime_error("Method is not allowed in meta");
    }

    if (!params().empty()) {
        throw std::runtime_error("Params is not allowed in meta");
    }

    if (xsltDefined()) {
        throw std::runtime_error("Xslt is not allowed in meta");
    }

    if (lua_block_.get()) {
        if (lua_block_->xsltDefined()) {
            throw std::runtime_error("Xslt is not allowed in meta lua");
        }
        if (lua_block_->metaBlock()) {
            throw std::runtime_error("Meta is not allowed in meta lua");
        }
    }

    if (hasGuard()) {
        throw std::runtime_error("Guard is not allowed in meta");
    }

    if (root_name_.empty()) {
        root_name_.assign("meta");
    }
    else {
        std::string::size_type pos = root_name_.find(':');
        if (std::string::npos != pos) {
            std::string prefix = root_name_.substr(0, pos);
            root_name_.erase(0, pos + 1);

            if (root_name_.empty()) {
                throw std::runtime_error("Empty name in name attribute is not allowed in meta");
            }
            if (!prefix.empty()) {
                const std::map<std::string, std::string> names = namespaces();
                std::map<std::string, std::string>::const_iterator it = names.find(prefix);
                if (names.end() == it) {
                    std::stringstream str;
                    str << "Unknown " << parent_->name() << " block namespace: " << prefix;
                    throw std::runtime_error(str.str());
                }
                root_ns_ = xmlSearchNsByHref(node()->doc, node(), (const xmlChar*)it->second.c_str());
                if (NULL == root_ns_) {
                    std::stringstream str;
                    str << "Cannot find " << parent_->name() << " block namespace: " << prefix;
                    throw std::runtime_error(str.str());
                }
            }
        }
        else {
            root_ns_ = NULL;
        }
    }

    if (before_cache_save_lua_.get()) {
        Range code = getLuaCode(before_cache_save_lua_.get());
        if (!code.empty()) {
            key_.push_back('|');
            key_.append(HashUtils::hexMD5(code.begin(), code.size()));
        }
    }
}

Range
MetaBlock::getLuaCode(Block *lua) const {
    const char* code = XmlUtils::cdataValue(lua->node());
    if (NULL == code) {
        code = XmlUtils::value(lua->node());
        if (NULL != code) {
            return trim(createRange(code));
        }
    }
    return Range();
}

const std::string&
MetaBlock::getTagKey() const {
    return key_;
}

bool
MetaBlock::haveCachedLua() const {
    return before_cache_save_lua_.get() || after_cache_load_lua_.get();
}

} // namespace xscript
