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

class ContextListStopper {
public:
    ContextListStopper()
    {}
    void add(boost::shared_ptr<Context> ctx) {
        stoppers_.push_back(new ContextStopper(ctx));
    }
    ~ContextListStopper() {
        for (std::list<ContextStopper*>::iterator it = stoppers_.begin();
             it != stoppers_.end();
             ++it) {
            if (NULL != *it) {
                delete *it;
            }
        }
    }
private:
    std::list<ContextStopper*> stoppers_;
};

ContextStopper::ContextStopper(boost::shared_ptr<Context> ctx) : ctx_(ctx) {
}

ContextStopper::~ContextStopper() {
    if (ctx_.get()) {
        ctx_->stop();
    }
}

void
ContextStopper::reset() {
    return ctx_.reset();
}

WhileBlock::WhileBlock(const Extension *ext, Xml *owner, xmlNodePtr node) :
    Block(ext, owner, node), LocalBlock(ext, owner, node)
{
    proxy_flags(Context::PROXY_ALL);
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

    bool no_cache = false;
    boost::shared_ptr<Context> main_child_ctx;
    std::list<boost::shared_ptr<Context> > ctxs;
    ContextListStopper ctx_list_stopper;
    boost::xtime end_time = Context::delay(0);
    while (1) {
        if (remainedTime(ctx.get()) <= 0) {
            InvokeError error("block is timed out");
            error.add("timeout", boost::lexical_cast<std::string>(ctx->timer().timeout()));
            throw error;
        }

        main_child_ctx = Context::createChildContext(
            script(), ctx, invoke_ctx, ctx->localParamsMap(), Context::PROXY_ALL);
        ctxs.push_back(main_child_ctx);

        ContextStopper ctx_stopper(main_child_ctx);
        boost::xtime tmp = script()->invokeBlocks(main_child_ctx);
        if (boost::xtime_cmp(tmp, end_time) > 0) {
            end_time = tmp;
        }

        if (!checkStateGuard(ctx.get())) {
            ctx_stopper.reset();
            break;
        }

        ctx_stopper.reset();
        ctx_list_stopper.add(main_child_ctx);
    }

    ContextStopper ctx_stopper(main_child_ctx);

    XmlDocSharedHelper doc;
    xmlNodePtr root = NULL;

    for (std::list<boost::shared_ptr<Context> >::iterator it = ctxs.begin();
         it != ctxs.end();
         ++it) {
        boost::shared_ptr<Context> local_ctx = *it;
        local_ctx->wait(end_time);
        XmlDocSharedHelper doc_tmp = script()->processResults(local_ctx);
        XmlUtils::throwUnless(NULL != doc_tmp.get());
        xmlNodePtr root_tmp = xmlDocGetRootElement(doc_tmp.get());
        XmlUtils::throwUnless(NULL != root_tmp);
        if (doc.get()) {
            xmlNodePtr next = root_tmp->children;
            while (next) {
                xmlAddChild(root, xmlCopyNode(next, 1));
                ctx->addDoc(doc_tmp);
                next = next->next;
            }
        }
        else {
            doc = doc_tmp;
            root = root_tmp;
        }

        if (local_ctx->noCache()) {
            no_cache = true;
        }

        if (main_child_ctx.get() != local_ctx.get() && !local_ctx->expireDeltaUndefined()) {
            main_child_ctx->setExpireDelta(local_ctx->expireDelta());
        }
    }

    if (no_cache) {
        invoke_ctx->resultType(InvokeContext::NO_CACHE);
    }

    invoke_ctx->resultDoc(doc);
    ctx_stopper.reset();
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
