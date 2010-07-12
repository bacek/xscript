#include "settings.h"

#include <sstream>

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
    proxy_(false), node_id_(XmlUtils::getUniqueNodeId(node)) {
}

LocalBlock::~LocalBlock() {
}

void
LocalBlock::propertyInternal(const char *name, const char *value) {
    if (!TaggedBlock::propertyInternal(name, value)) {
        ThreadedBlock::property(name, value);
    }
}

void
LocalBlock::property(const char *name, const char *value) {
    if (strncasecmp(name, "proxy", sizeof("proxy")) == 0) {
        proxy_ = (strncasecmp(value, "yes", sizeof("yes")) == 0);
    }
    else {
        propertyInternal(name, value);
    }
}

void
LocalBlock::call(boost::shared_ptr<Context> ctx,
    boost::shared_ptr<InvokeContext> invoke_ctx) const throw (std::exception) {
    
    if (invoke_ctx->haveCachedCopy()) {
        Tag local_tag = invoke_ctx->tag();
        local_tag.modified = false;
        invoke_ctx->tag(local_tag);
        invoke_ctx->resultDoc(XmlDocHelper());
        return;
    }
    
    const std::vector<Param*>& params = this->params();
    boost::shared_ptr<TypedMap> local_params(new TypedMap());
    LocalArgList param_list;
    for(std::vector<Param*>::const_iterator it = params.begin();
        it != params.end();
        ++it) {
        (*it)->add(ctx.get(), param_list);
        local_params->insert((*it)->id(), param_list.value());
    }

    boost::shared_ptr<Context> local_ctx =
        Context::createChildContext(script_, ctx, invoke_ctx, local_params, proxy_);

    ContextStopper ctx_stopper(local_ctx);
    
    if (threaded() || ctx->forceNoThreaded()) {
        local_ctx->forceNoThreaded(true);
    }
    
    XmlDocSharedHelper doc = script_->invoke(local_ctx);
    XmlUtils::throwUnless(NULL != doc.get());
    
    if (local_ctx->noCache()) {
        invoke_ctx->resultType(InvokeContext::NO_CACHE);
    }
    
    ctx_stopper.reset();
    invoke_ctx->resultDoc(doc);
    if (tagged()) {
        const Xml::TimeMapType& modified_info = script_->modifiedInfo();
        time_t max_time = 0;
        for (Xml::TimeMapType::const_iterator it = modified_info.begin();
             it != modified_info.end();
             ++it) {
            max_time = std::max(max_time, it->second);
        }
        Tag local_tag(true, max_time, Tag::UNDEFINED_TIME);
        invoke_ctx->tag(local_tag);
    }
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
            std::string node_name, prefix;
            const char* ch = strchr(value, ':');
            if (NULL == ch) {
                node_name.assign(value);
            }
            else {
                prefix.assign(value, ch - value);
                node_name.assign(ch + 1);
            }
            if (node_name.empty()) {
                std::stringstream str;
                str << "Empty root node name is not allowed in " << name() << " block";
                throw std::runtime_error(str.str());
            }
            xmlNodeSetName(node, (const xmlChar*)node_name.c_str());
            xmlNsPtr ns = NULL;
            if (!prefix.empty()) {
                const std::map<std::string, std::string> names = namespaces();
                std::map<std::string, std::string>::const_iterator it = names.find(prefix);
                if (names.end() == it) {
                    std::stringstream str;
                    str << "Unknown " << name() << " block namespace: " << prefix;
                    throw std::runtime_error(str.str());
                }
                ns = xmlSearchNsByHref(node->doc, node, (const xmlChar*)it->second.c_str());
                if (NULL == ns) {
                    std::stringstream str;
                    str << "Cannot find " << name() << " block namespace: " << prefix;
                    throw std::runtime_error(str.str());
                }
            }
            xmlSetNs(node, ns);
        }
        xmlRemoveProp(name_attr);
    }

    script_ = ScriptFactory::createScriptFromXmlNode(owner()->name(), node);
    Xml::TimeMapType modified_info = owner()->modifiedInfo();
    script_->swapModifiedInfo(modified_info);
}

void
LocalBlock::postParseInternal() {
    if (NULL == script_.get()) {
        std::stringstream str;
        str << "Child script is not specified in " << name() << " block";
        throw std::runtime_error(str.str());
    }
    
    TaggedBlock::postParse();
}

void
LocalBlock::postParse() {
    postParseInternal();
    const std::vector<Param*>& params = this->params();
    for(std::vector<Param*>::const_iterator it = params.begin();
        it != params.end();
        ++it) {
        if ((*it)->id().empty()) {
            throw std::runtime_error("local block param without id");
        }
    }
    createCanonicalMethod("local.");
}

void
LocalBlock::proxy(bool flag) {
    proxy_ = flag;
}

bool
LocalBlock::proxy() const {
    return proxy_;
}

boost::shared_ptr<Script>
LocalBlock::script() const {
    return script_;
}

std::string
LocalBlock::createTagKey(const Context *ctx, const InvokeContext *invoke_ctx) const {
    std::string key(processMainKey(ctx, invoke_ctx));
    key.push_back('|');
    key.append(paramsIdKey(params(), ctx));
    key.push_back('|');
    key.append(script_->name());
    key.push_back('|');
    key.append(modifiedKey(script_->modifiedInfo()));
    key.push_back('|');
    key.append(blocksModifiedKey(script_->blocks()));
    key.push_back('|');
    key.append(node_id_);
    return key;
}

} // namespace xscript
