#include "settings.h"

#include <xscript/context.h>
#include <xscript/script.h>
#include <xscript/typed_map.h>
#include <xscript/xml_util.h>

#include "while_block.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

WhileBlock::WhileBlock(const Extension *ext, Xml *owner, xmlNodePtr node) :
    Block(ext, owner, node), LocalBlock(ext, owner, node)
{
    proxy(true);
}

WhileBlock::~WhileBlock() {
}

void
WhileBlock::property(const char *name, const char *value) {
    LocalBlock::propertyInternal(name, value);
}

void
WhileBlock::call(boost::shared_ptr<Context> ctx,
    boost::shared_ptr<InvokeContext> invoke_ctx) const throw (std::exception) {
    
    if (invoke_ctx->haveCachedCopy()) {
        Tag local_tag = invoke_ctx->tag();
        local_tag.modified = false;
        invoke_ctx->tag(local_tag);
        invoke_ctx->resultDoc(XmlDocHelper());
        return;
    }

    XmlDocSharedHelper doc;
    xmlNodePtr root = NULL;
    bool no_threaded = threaded() || ctx->forceNoThreaded();
    do {
        if (remainedTime(ctx.get()) <= 0) {
            InvokeError error("block is timed out");
            error.add("timeout", boost::lexical_cast<std::string>(ctx->timer().timeout()));
            throw error;
        }

        boost::shared_ptr<Context> local_ctx =
            Context::createChildContext(script(), ctx, invoke_ctx, ctx->localParamsMap(), true);
        invoke_ctx->setLocalContext(boost::shared_ptr<Context>());

        ContextStopper ctx_stopper(local_ctx);
        if (no_threaded) {
            local_ctx->forceNoThreaded(no_threaded);
        }

        XmlDocSharedHelper doc_iter = script()->invoke(local_ctx);
        XmlUtils::throwUnless(NULL != doc_iter.get());
        xmlNodePtr root_local = xmlDocGetRootElement(doc_iter.get());
        XmlUtils::throwUnless(NULL != root_local);

        if (doc.get()) {
            xmlNodePtr next = root_local->children;
            while (next) {
                xmlAddChild(root, xmlCopyNode(next, 1));
                ctx->addDoc(doc_iter);
                next = next->next;
            }
        }
        else {
            doc = doc_iter;
            root = root_local;
        }

        if (local_ctx->noCache()) {
            invoke_ctx->resultType(InvokeContext::NO_CACHE);
        }

    } while(checkStateGuard(ctx.get()));

    invoke_ctx->resultDoc(doc);
}

void
WhileBlock::postParse() {
    postParseInternal();
    if (!params().empty()) {
        throw std::runtime_error("params is not allowed in while block");
    }

    if (!hasStateGuard()) {
        throw std::runtime_error("state guard should be specified in while block");
    }

    createCanonicalMethod("while.");
}

} // namespace xscript
