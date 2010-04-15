#include "settings.h"

#include <boost/current_function.hpp>

#include <xscript/context.h>
#include <xscript/logger.h>
#include <xscript/param.h>
#include <xscript/script.h>
#include <xscript/script_factory.h>
#include <xscript/typed_map.h>
#include <xscript/xml_util.h>

#include "local_arg_list.h"
#include "local_block.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

LocalBlock::LocalBlock(const Extension *ext, Xml *owner, xmlNodePtr node) :
    Block(ext, owner, node), ThreadedBlock(ext, owner, node), TaggedBlock(ext, owner, node),
    proxy_(false),
    node_count_(boost::lexical_cast<std::string>(XmlUtils::getNodeCount(node))) {
}

LocalBlock::~LocalBlock() {
}

void
LocalBlock::property(const char *name, const char *value) {
    if (strncasecmp(name, "proxy", sizeof("proxy")) == 0) {
        proxy_ = (strncasecmp(value, "yes", sizeof("yes")) == 0);
    }
    else if (!TaggedBlock::propertyInternal(name, value)) {
        ThreadedBlock::property(name, value);
    }
}

XmlDocHelper
LocalBlock::call(boost::shared_ptr<Context> ctx,
    boost::shared_ptr<InvokeContext> invoke_ctx) throw (std::exception) {
    
    if (invoke_ctx->haveCachedCopy()) {
        Tag local_tag = invoke_ctx->tag();
        local_tag.modified = false;
        invoke_ctx->tag(local_tag);
        return XmlDocHelper();
    }
    
    const std::vector<Param*>& params = this->params();
    TypedMap local_params;
    LocalArgList param_list;
    for(std::vector<Param*>::const_iterator it = params.begin();
        it != params.end();
        ++it) {
        (*it)->add(ctx.get(), param_list);
        local_params.insert((*it)->id(), param_list.value());
    }

    boost::shared_ptr<Context> local_ctx =
        Context::createChildContext(script_, ctx, local_params, proxy_);

    ContextStopper ctx_stopper(local_ctx);
    
    invoke_ctx->setLocalContext(local_ctx);
    
    if (threaded() || ctx->forceNoThreaded()) {
        local_ctx->forceNoThreaded(true);
    }
    
    XmlDocSharedHelper doc = script_->invoke(local_ctx);
    XmlUtils::throwUnless(NULL != doc->get());
    
    if (local_ctx->noCache()) {
        invoke_ctx->resultType(InvokeContext::NO_CACHE);
    }
    
    ctx_stopper.reset();
    
    return *doc;
}

void
LocalBlock::parseSubNode(xmlNodePtr node) {
    if (NULL == node->name || 0 != xmlStrcasecmp(node->name, (const xmlChar*)"root")) {
        Block::parseSubNode(node);
        return;
    }

    const xmlChar* ref = node->ns ? node->ns->href : NULL;
    if (NULL != ref && 0 != xmlStrcasecmp(ref, (const xmlChar*)XmlUtils::XSCRIPT_NAMESPACE)) {
        Block::parseSubNode(node);
        return;
    }

    xmlAttrPtr name_attr = xmlHasProp(node, (const xmlChar*)"name");
    if (name_attr) {
        const char* value = XmlUtils::value(name_attr);
        if (NULL != value) {
            std::string name, prefix;
            const char* ch = strchr(value, ':');
            if (NULL == ch) {
                name.assign(value);
            }
            else {
                prefix.assign(value, ch - value);
                name.assign(ch + 1);
            }
            if (name.empty()) {
                throw std::runtime_error("Empty root node name is not allowed in local block");
            }
            xmlNodeSetName(node, (const xmlChar*)name.c_str());
            xmlNsPtr ns = NULL;
            if (!prefix.empty()) {
                const std::map<std::string, std::string> names = namespaces();
                std::map<std::string, std::string>::const_iterator it = names.find(prefix);
                if (names.end() == it) {
                    throw std::runtime_error("Unknown local block namespace: " + prefix);
                }
                ns = xmlSearchNsByHref(node->doc, node, (const xmlChar*)it->second.c_str());
                if (NULL == ns) {
                    throw std::runtime_error("Cannot find local block namespace: " + prefix);
                }
            }
            xmlSetNs(node, ns);
        }
        xmlRemoveProp(name_attr);
    }

    script_ = ScriptFactory::createScriptFromXmlNode(owner()->name(), node);
}

void
LocalBlock::postParse() {
    if (NULL == script_.get()) {
        throw CriticalInvokeError("script is not specified in local block");
    }
    
    const std::vector<Param*>& params = this->params();
    for(std::vector<Param*>::const_iterator it = params.begin();
        it != params.end();
        ++it) {
        if ((*it)->id().empty()) {
            throw std::runtime_error("local block param without id");
        }
    }
    
    TaggedBlock::postParse();
}

std::string
LocalBlock::createTagKey(const Context *ctx) const {
    std::string key(processMainKey(ctx));
    key.push_back('|');
    key.append(paramsIdKey(params(), ctx));
    key.push_back('|');
    key.append(owner()->name());
    key.push_back('|');
    key.append(modifiedKey(owner()->modifiedInfo()));
    key.push_back('|');
    key.append(blocksModifiedKey(script_->blocks()));
    key.push_back('|');
    key.append(node_count_);
    return key;
}

} // namespace xscript
