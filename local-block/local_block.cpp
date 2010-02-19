#include "settings.h"

#include <boost/current_function.hpp>

#include <xscript/context.h>
#include <xscript/logger.h>
#include <xscript/param.h>
#include <xscript/script.h>
#include <xscript/script_factory.h>
#include "xscript/typed_map.h"
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

    if (threaded() || ctx->forceNoThreaded()) {
        local_ctx->forceNoThreaded(true);
    }

    ContextStopper ctx_stopper(local_ctx);
    
    XmlDocSharedHelper doc = script_->invoke(local_ctx);
    XmlUtils::throwUnless(NULL != doc->get());
    
    if (local_ctx->noCache()) {
        invoke_ctx->resultType(InvokeContext::NO_CACHE);
    }

    if (tagged()) {
        Tag tag;
        tag.last_modified = time(NULL);
        if (!proxy_) {
            tag.expire_time = tag.last_modified + local_ctx->response()->expireDelta();
        }
        invoke_ctx->tag(tag);
    }
    
    return *doc;
}

void
LocalBlock::parseSubNode(xmlNodePtr node) {
    if (!node->name || xmlStrncasecmp(node->name, (const xmlChar*)"root", sizeof("root")) != 0) {
        Block::parseSubNode(node);
        return;
    }
    
    xmlAttrPtr name_attr = xmlHasProp(node, (const xmlChar*)"name");
    if (name_attr) {
        const xmlChar* value = (const xmlChar*)XmlUtils::value(name_attr);
        if (NULL != value) {
            xmlNodeSetName(node, value);
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
