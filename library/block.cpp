#include "settings.h"

#include <cassert>
#include <sstream>
#include <stdexcept>
#include <algorithm>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/checked_delete.hpp>
#include <boost/current_function.hpp>

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "internal/block_helpers.h"
#include "internal/param_factory.h"

#include "xscript/args.h"
#include "xscript/block.h"
#include "xscript/context.h"
#include "xscript/logger.h"
#include "xscript/meta.h"
#include "xscript/meta_block.h"
#include "xscript/object.h"
#include "xscript/operation_mode.h"
#include "xscript/param.h"
#include "xscript/policy.h"
#include "xscript/profiler.h"
#include "xscript/script.h"
#include "xscript/state.h"
#include "xscript/stylesheet.h"
#include "xscript/stylesheet_factory.h"
#include "xscript/thread_pool.h"
#include "xscript/vhost_data.h"
#include "xscript/xml.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

InvokeHelper::InvokeHelper(boost::shared_ptr<Context> ctx) : ctx_(ctx) {
    boost::shared_ptr<Context> ctx_tmp = ctx->parentContext();
    while (ctx_tmp.get()) {
        parents_.push_back(ctx_tmp);
        ctx_tmp = ctx_tmp->parentContext();
    }
}

InvokeHelper::~InvokeHelper() {
    ctx_.reset();
    while (!parents_.empty()) {
        parents_.pop_front();
    }
}

const boost::shared_ptr<Context>&
InvokeHelper::context() const {
    return ctx_;
}

struct Block::BlockData {
    BlockData(const Extension *ext, Xml *owner, xmlNodePtr node, Block *block);
    ~BlockData();
    void parseNamespaces();
    void property(const char *name, const char *value);
    
    const Extension *extension_;
    Xml *owner_;
    xmlNodePtr node_;
    Block *block_;
    std::vector<Param*> params_;
    std::list<XPathNodeExpr> xpath_;
    std::list<Guard> guards_;
    std::string id_, method_;
    boost::shared_ptr<XPathExpr> xpointer_expr_;
    std::string base_;
    std::map<std::string, std::string> namespaces_;
    bool disable_output_;

    std::auto_ptr<MetaBlock> meta_block_;

    static const std::string XSCRIPT_INVOKE_FAILED;
    static const std::string META_INVOKE_FAILED;
    static const std::string XSCRIPT_INVOKE_INFO;
};

const std::string Block::BlockData::XSCRIPT_INVOKE_FAILED = "xscript_invoke_failed";
const std::string Block::BlockData::META_INVOKE_FAILED = "meta_invoke_failed";
const std::string Block::BlockData::XSCRIPT_INVOKE_INFO = "xscript_invoke_info";

Block::BlockData::BlockData(const Extension *ext, Xml *owner, xmlNodePtr node, Block *block) :
    extension_(ext), owner_(owner), node_(node), block_(block), xpointer_expr_(new XPathExpr()),
    disable_output_(false)
{}

Block::BlockData::~BlockData() {
    std::for_each(params_.begin(), params_.end(), boost::checked_deleter<Param>());
}

void
Block::BlockData::parseNamespaces() {

    for (xmlNodePtr node = node_; NULL != node; node = node->parent) {
        if (node->type == XML_ELEMENT_NODE) {
            for(xmlNs *ns = node->nsDef; NULL != ns; ns = ns->next) {
                if (ns->prefix && ns->href) {
                    namespaces_.insert(std::make_pair(
                        std::string((const char*)ns->prefix), std::string((const char*)ns->href)));
                }
            }
        }
    }
}

void
Block::BlockData::property(const char *name, const char *value) {

    block_->log()->debug("setting %s=%s", name, value);

    if (strncasecmp(name, "id", sizeof("id")) == 0) {
        if (!id_.empty()) {
            throw std::runtime_error("duplicated id in block");
        }
        id_.assign(value);
    }
    else if (strncasecmp(name, "guard", sizeof("guard")) == 0) {
        if (*value == '\0') {
            throw std::runtime_error("empty guard");
        }
        guards_.push_back(Guard(value, NULL, NULL, false));
    }
    else if (strncasecmp(name, "guard-not", sizeof("guard-not")) == 0) {
        if (*value == '\0') {
            throw std::runtime_error("empty guard-not");
        }
        guards_.push_back(Guard(value, NULL, NULL, true));
    }
    else if (strncasecmp(name, "method", sizeof("method")) == 0) {
        if (!method_.empty()) {
            throw std::runtime_error("duplicated method in block");
        }
        method_.assign(value);
    }
    else if (strncasecmp(name, "xslt", sizeof("xslt")) == 0) {
        block_->xsltName(value, NULL);
    }
    else if (strncasecmp(name, "xpointer", sizeof("xpointer")) == 0) {
        block_->parseXPointerExpr(value, NULL);
    }
    else {
        std::stringstream stream;
        stream << "bad block attribute: " << name;
        throw std::invalid_argument(stream.str());
    }
}

Block::Block(const Extension *ext, Xml *owner, xmlNodePtr node) :
    data_(new BlockData(ext, owner, node, this))
{
    assert(node);
    assert(owner);
}

Block::~Block() {
}

Xml*
Block::owner() const {
    return data_->owner_;
}

xmlNodePtr
Block::node() const {
    return data_->node_;
}

const std::string&
Block::id() const {
    return data_->id_;
}

const std::string&
Block::method() const {
    return data_->method_;
}

const char*
Block::name() const {
    return data_->extension_->name();
}

const Param*
Block::param(const std::string &id, bool throw_error) const {
    try {
        for(std::vector<Param*>::const_iterator i = data_->params_.begin(), end = data_->params_.end();
            i != end;
            ++i) {
            
            if ((*i)->id() == id) {
                return (*i);
            }
        }
        if (throw_error) {
            throw std::invalid_argument("param with id not found: " + id);
        }
        return NULL;
    }
    catch (const std::exception &e) {
        log()->error("%s, caught exception: %s", owner()->name().c_str(), e.what());
        throw;
    }
}

const Param*
Block::param(unsigned int n) const {
    return data_->params_.at(n);
}

const std::vector<Param*>&
Block::params() const {
    return data_->params_;
}

bool
Block::threaded() const {
    return false;
}

void
Block::threaded(bool) {
}

bool
Block::tagged() const {
    return false;
}

// return last insert node
xmlNodePtr
Block::processEmptyXPointer(const InvokeContext *invoke_ctx, xmlDocPtr meta_doc,
        xmlNodePtr insert_node, insertFunc func) const {
    if (meta_doc && data_->meta_block_.get()) {
        return data_->meta_block_->processXPointer(
            invoke_ctx, meta_doc, NULL, insert_node, func);
    }
    return NULL;
}

// return last insert node
xmlNodePtr
Block::processXPointer(const InvokeContext *invoke_ctx, xmlDocPtr doc, xmlDocPtr meta_doc,
        xmlNodePtr insert_node, insertFunc func) const {
    if (NULL == doc || data_->disable_output_) {
        return processEmptyXPointer(invoke_ctx, meta_doc, insert_node, func);
    }
    xmlNodePtr root_node = xmlDocGetRootElement(doc);

    boost::shared_ptr<XPathExpr> xpointer = isMeta() ?
        invoke_ctx->metaXPointer() : invoke_ctx->xpointer();

    if (NULL == xpointer.get()) {
        xmlNodePtr node = xmlCopyNode(root_node, 1);
        func(insert_node, node);
        if (meta_doc && data_->meta_block_.get()) {
            return data_->meta_block_->processXPointer(
                invoke_ctx, meta_doc, NULL, node, &xmlAddNextSibling);
        }
        return node;
    }

    if (xpointer->stripAll()) {
        return processEmptyXPointer(invoke_ctx, meta_doc, insert_node, func);
    }

    XmlXPathContextHelper context(xmlXPathNewContext(doc));
    XmlUtils::throwUnless(NULL != context.get());
    //context->node = root_node && root_node->parent ? root_node->parent : root_node;
    context->node = root_node;

    XmlUtils::regiserNsList(context.get(), data_->namespaces_);
    XmlXPathObjectHelper xpathObj = xpointer->eval(context.get(), NULL);

    xmlNodePtr node = NULL;
    if (XPATH_BOOLEAN == xpathObj->type) {
        const char *str = 0 != xpathObj->boolval ? "1" : "0";
        node = xmlNewTextLen((const xmlChar *)str, 1);
    }
    else if (XPATH_NUMBER == xpathObj->type) {
        char str[40];
        snprintf(str, sizeof(str) - 1, "%f", xpathObj->floatval);
        str[sizeof(str) - 1] = '\0';
        node = xmlNewText((const xmlChar *)&str[0]);
    }
    else if (XPATH_STRING == xpathObj->type) {
        node = xmlNewText((const xmlChar *)xpathObj->stringval);
    }

    if (node) {
        func(insert_node, node);
        if (meta_doc && data_->meta_block_.get()) {
            return data_->meta_block_->processXPointer(
                invoke_ctx, meta_doc, NULL, node, &xmlAddNextSibling);
        }
        return node;
    }

    xmlNodeSetPtr nodeset = xpathObj->nodesetval;
    if (NULL == nodeset || 0 == nodeset->nodeNr) {
        return processEmptyXPointer(invoke_ctx, meta_doc, insert_node, func);
    }

    xmlNodePtr current_node = nodeset->nodeTab[0];
    if (XML_ATTRIBUTE_NODE == current_node->type) {
        current_node = current_node->children;
    }
    node = xmlCopyNode(current_node, 1);
    func(insert_node, node);
    xmlNodePtr last_input_node = node;
    for (int i = 1; i < nodeset->nodeNr; ++i) {
        xmlNodePtr current_node = nodeset->nodeTab[i];
        if (XML_ATTRIBUTE_NODE == current_node->type) {
            current_node = current_node->children;
        }
        xmlNodePtr node = xmlCopyNode(current_node, 1);
        xmlAddNextSibling(last_input_node, node);
        last_input_node = node;
    }

    if (meta_doc && data_->meta_block_.get()) {
        return data_->meta_block_->processXPointer(
            invoke_ctx, meta_doc, NULL, last_input_node, &xmlAddNextSibling);
    }

    return last_input_node;
}

void
Block::detectBase() {
    XmlCharHelper base(xmlNodeGetBase(data_->node_->doc, data_->node_));
    if (NULL == base.get()) {
        return;
    }

    data_->base_ = (const char*)base.get();
    std::string::size_type pos = data_->base_.find_last_of('/');
    if (pos != std::string::npos) {
        data_->base_.erase(pos + 1);
    }
}

const std::string&
Block::getBase() const {
    return data_->base_;
}

const std::map<std::string, std::string>&
Block::namespaces() const {
    return data_->namespaces_;
}

void
Block::addNamespaces(const std::map<std::string, std::string> &ns) {
    data_->namespaces_.insert(ns.begin(), ns.end());
}

const Extension*
Block::extension() const {
    return data_->extension_;
}

void
Block::parse() {
    try {       
        detectBase();
        data_->parseNamespaces();
        
        XmlUtils::visitAttributes(data_->node_->properties,
            boost::bind(&Block::property, this, _1, _2));
        
        for (xmlNodePtr node = data_->node_->children; NULL != node; node = node->next) {
            parseSubNode(node);
        }
        postParse();
    }
    catch (ParseError &e) {
        e.add(errorLocation());
        throw e;
    }
    catch (const std::exception &e) {
        ParseError error(e.what());
        error.add(errorLocation());
        throw error;
    }
}

void
Block::parseSubNode(xmlNodePtr node) {
    if (node->name) {
        if (xpathNode(node)) {
            parseXPathNode(node);
        }
        else if (xpointerNode(node)) {
            parseXPointerNode(node);
        }
        else if (paramNode(node)) {
            parseParamNode(node);
        }
        else if (xsltParamNode(node)) {
            parseXsltParamNode(node);
        }
        else if (metaNode(node)) {
            parseMetaNode(node);
        }
        else if (xsltNode(node)) {
            parseXsltNode(node);
        }
        else if (guardNode(node)) {
            parseGuardNode(node, false);
        }
        else if (guardNotNode(node)) {
            parseGuardNode(node, true);
        }
        else if (XML_ELEMENT_NODE == node->type) {
            const char *value = XmlUtils::value(node);
            if (value) {
                property((const char*) node->name, value);
            }
        }
    }
}

std::string
Block::fullName(const std::string &name) const {
    if (strncasecmp(name.c_str(), "xmlbase://", sizeof("xmlbase://") - 1) != 0) {
        return owner()->fullName(name);
    }
    std::string name_tmp = data_->base_ + name.substr(sizeof("xmlbase://") - 1);
    return owner()->fullName(name_tmp);
}

boost::shared_ptr<InvokeContext>
Block::invoke(boost::shared_ptr<Context> ctx) {
    boost::shared_ptr<InvokeContext> invoke_ctx = createInvokeContext(ctx);
    return invoke_Ex(ctx, invoke_ctx);
}

boost::shared_ptr<InvokeContext>
Block::invoke_Ex(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const {
    log()->entering(BOOST_CURRENT_FUNCTION);

    if (ctx->stopped()) {
        log()->info("Context already stopped. Cannot invoke block. Owner: %s. Block: %s. Method: %s",
            owner()->name().c_str(), name(), data_->method_.c_str());
        ctx->setNoCache();
        return fakeResult(false);
    }

    try {
        BlockTimerStarter starter(ctx.get(), this);
        Block *self = const_cast<Block*>(this); //TODO: remove const_cast
        self->invokeInternal(ctx, invoke_ctx);
        try {
            self->callMetaLua(ctx, invoke_ctx);
        }
        catch (const MetaInvokeError &e) {
            self->postInvoke(ctx.get(), invoke_ctx.get());
            throw e;
        }
        self->postInvoke(ctx.get(), invoke_ctx.get());
        self->callMeta(ctx, invoke_ctx);
    }
    catch (const CriticalInvokeError &e) {
        std::string full_error;
        invoke_ctx = errorResult(e, full_error);
        OperationMode::instance()->assignBlockError(ctx.get(), this, full_error);
    }
    catch (const SkipResultInvokeError &e) {
        log()->info("suppressed : %s", errorMessage(e).c_str());
        invoke_ctx = fakeResult(false);
    }
    catch (const MetaInvokeError &e) {
        metaErrorResult(e, invoke_ctx);
    }
    catch (const InvokeError &e) {
        invoke_ctx = errorResult(e, false);
    }
    catch (const std::exception &e) {
        invoke_ctx = errorResult(e.what(), false);
    }
    
    if (!invoke_ctx->success()) {
        ctx->setNoCache();
    }
    
    return invoke_ctx;
}

void
Block::invokeInternal(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) {
    log()->entering(BOOST_CURRENT_FUNCTION);

    // Check validators for each param before calling it.
    for(std::vector<Param*>::const_iterator i = data_->params_.begin();
        i != data_->params_.end();
        ++i) {
        (*i)->checkValidator(ctx.get());
    }
    
    call(ctx, invoke_ctx);
    if (NULL == invoke_ctx->resultDoc().get()) {
        errorResult("got empty document", false, invoke_ctx);
        return;
    }
    processResponse(ctx, invoke_ctx);
}

void
Block::callInternalThreadedHelper(InvokeHelper helper, boost::shared_ptr<InvokeContext> invoke_ctx, unsigned int slot) const {
    callInternalThreaded_Ex(helper.context(), invoke_ctx, slot);
}

void
Block::invokeCheckThreaded(boost::shared_ptr<Context> ctx, unsigned int slot) {
    (void)ctx;
    (void)slot;
    assert(false);
}

boost::shared_ptr<InvokeContext>
Block::createInvokeContext(boost::shared_ptr<Context> ctx) const {
    boost::shared_ptr<InvokeContext> invoke_ctx = boost::shared_ptr<InvokeContext>(new InvokeContext());
    invoke_ctx->xsltName(xsltName(ctx.get()));
    processArguments(ctx.get(), invoke_ctx.get());
    return invoke_ctx;
}

bool
Block::invokeCheckThreadedEx(boost::shared_ptr<Context> ctx, unsigned int slot) const {
    boost::shared_ptr<InvokeContext> invoke_ctx = createInvokeContext(ctx);
    if (threaded()) {
        InvokeHelper helper(ctx);
        boost::function<void()> f_threaded = boost::bind(&Block::callInternalThreadedHelper, this, helper, invoke_ctx, slot);
        boost::function<void()> f_unthreaded = boost::bind(&Block::callInternal_Ex, this, ctx, invoke_ctx, slot);
        return ThreadPool::instance()->invokeEx(f_threaded, f_unthreaded);
    }
    callInternal_Ex(ctx, invoke_ctx, slot);
    return false;
}

void
Block::processResponse(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) { 
    XmlDocSharedHelper doc = invoke_ctx->resultDoc();
    if (NULL == doc.get()) {
        throw InvokeError("null response document");
    }

    if (NULL == xmlDocGetRootElement(doc.get())) {
        throw InvokeError("got document with no root");
    }

    if (!tagged() && ctx->stopped()) {
        throw InvokeError("context is already stopped, cannot process response");
    }

    bool is_error_doc = Policy::instance()->isErrorDoc(doc.get());
    
    log()->debug("%s, got source document: %p", BOOST_CURRENT_FUNCTION, doc.get());

    boost::shared_ptr<Context> local_ctx = invoke_ctx->getLocalContext();
    bool success = applyStylesheet(local_ctx.get() ? local_ctx : ctx, invoke_ctx, doc);

    InvokeContext::ResultType type = InvokeContext::SUCCESS;    
    if (is_error_doc || !success || invoke_ctx->noCache()) {
        type = InvokeContext::NO_CACHE;
    }
    
    invoke_ctx->resultType(type);
    invoke_ctx->resultDoc(doc);

    if (metaBlock()) {
        invoke_ctx->meta()->setElapsedTime(ctx->timer().elapsed());
    }
    
    postCall(ctx, invoke_ctx);
}

bool
Block::applyStylesheet(boost::shared_ptr<Context> ctx,
    boost::shared_ptr<InvokeContext> invoke_ctx, XmlDocSharedHelper &doc) {
    if (ctx->noXsltPort()) {
        return true;
    }
    const std::string& xslt = invoke_ctx->xsltName();
    if (xslt.empty()) {
        return true;
    }
    const TimeoutCounter &timer = ctx->timer();
    if (timer.expired()) {
        throw InvokeError("block is timed out", "timeout",
            boost::lexical_cast<std::string>(timer.timeout()));
    }
    boost::shared_ptr<Stylesheet> sh = StylesheetFactory::createStylesheet(xslt);
    {
        std::stringstream str;
        str << "per-block-xslt: " << xslt;
        str << ", block: " << name();
        if (!id().empty()) {
            str << ", block-id: " << id();
        }
        if (!method().empty()) {
            str << ", method: " << method();
        }
        str << ", owner: " << owner()->name();
        PROFILER(log(), str.str())
        Object::applyStylesheet(sh, ctx, invoke_ctx, doc, XmlUtils::xmlVersionNumber() < 20619);
    }

    XmlUtils::throwUnless(NULL != doc.get());
    log()->debug("%s, got source document: %p", BOOST_CURRENT_FUNCTION, doc.get());

    bool result = true;
    if (XmlUtils::hasXMLError()) {
        result = false;
    }

    OperationMode::instance()->processPerblockXsltError(ctx.get(), invoke_ctx.get(), this);

    return result;
}

// TODO: remove this
bool
Block::applyStylesheet(boost::shared_ptr<Context> ctx, XmlDocSharedHelper &doc) {
    (void)ctx;
    (void)doc;
    return false;
}

void
Block::errorResult(const char *error, bool info, boost::shared_ptr<InvokeContext> &invoke_ctx) const {
    std::string full_error;
    InvokeError invoke_error(error);
    XmlDocHelper doc = errorDoc(invoke_error, info ? BlockData::XSCRIPT_INVOKE_INFO.c_str() :
        BlockData::XSCRIPT_INVOKE_FAILED.c_str(), full_error);
    
    invoke_ctx->resultDoc(doc);
    invoke_ctx->metaDoc(XmlDocSharedHelper());
    invoke_ctx->resultType(InvokeContext::ERROR);
}

boost::shared_ptr<InvokeContext>
Block::errorResult(const char *error, bool info) const {
    boost::shared_ptr<InvokeContext> result(new InvokeContext());
    errorResult(error, info, result);
    return result;
}

boost::shared_ptr<InvokeContext>
Block::errorResult(const InvokeError &error, bool info) const {
    std::string full_error;
    if (info) {
        return errorResult(errorDoc(error, BlockData::XSCRIPT_INVOKE_INFO.c_str(), full_error));    
    }
    return errorResult(errorDoc(error, BlockData::XSCRIPT_INVOKE_FAILED.c_str(), full_error));
}

void
Block::metaErrorResult(const MetaInvokeError &error,
    boost::shared_ptr<InvokeContext> &invoke_ctx) const {
    std::string full_error;
    XmlDocHelper doc(errorDoc(error, BlockData::META_INVOKE_FAILED.c_str(), full_error));
    invoke_ctx->metaDoc(doc);
    invoke_ctx->resultType(InvokeContext::META_ERROR);
}

boost::shared_ptr<InvokeContext>
Block::errorResult(const InvokeError &error, std::string &full_error) const {
    return errorResult(errorDoc(error, BlockData::XSCRIPT_INVOKE_FAILED.c_str(), full_error));
}

boost::shared_ptr<InvokeContext>
Block::errorResult(XmlDocHelper doc) const {
    boost::shared_ptr<InvokeContext> invoke_ctx(new InvokeContext());
    invoke_ctx->resultDoc(doc);
    invoke_ctx->resultType(InvokeContext::ERROR);
    return invoke_ctx;
}

boost::shared_ptr<InvokeContext>
Block::fakeResult(bool error) const {   
    boost::shared_ptr<InvokeContext> invoke_ctx(new InvokeContext());
    invoke_ctx->resultDoc(fakeDoc());
    invoke_ctx->resultType(error ? InvokeContext::ERROR : InvokeContext::SUCCESS);
    return invoke_ctx;
}

XmlDocHelper
Block::errorDoc(const InvokeError &error,
                const char *tag_name,
                std::string &full_error) const {
    
    XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != doc.get());
    
    XmlNodeHelper main_node(xmlNewDocNode(doc.get(), NULL, (const xmlChar*)tag_name, NULL));
    XmlUtils::throwUnless(NULL != main_node.get());
    
    xmlNewProp(main_node.get(), (const xmlChar*)"error", (const xmlChar*)error.what());
    std::stringstream stream;
    
    xmlNewProp(main_node.get(), (const xmlChar*)"block", (const xmlChar*)name());
    stream << "block: " << name() << " ";
    
    if (!method().empty()) {
        xmlNewProp(main_node.get(), (const xmlChar*)"method", (const xmlChar*)method().c_str());
        stream << "method: " << method() << " ";
    }

    if (!id().empty()) {
        xmlNewProp(main_node.get(), (const xmlChar*)"id", (const xmlChar*)id().c_str());
        stream << "id: " << id() << " ";
    }

    const InvokeError::InfoMapType& info = error.info();
    for(InvokeError::InfoMapType::const_iterator it = info.begin();
        it != info.end();
        ++it) {
        if (!it->second.empty()) {
            xmlNewProp(main_node.get(), (const xmlChar*)it->first.c_str(), (const xmlChar*)it->second.c_str());
            stream << it->first << ": " << it->second << " ";
        }
    }

    stream << "owner: " << owner()->name();

    XmlNodeHelper node = error.what_node();
    if (node.get()) {
        xmlAddChild(main_node.get(), node.release());
    }
    
    xmlDocSetRootElement(doc.get(), main_node.release());
    
    const std::string err = stream.str();
    const std::string &error_str = error.whatStr();

    full_error.reserve(error_str.size() + 1 + err.size());
    full_error.assign(error_str).append("\n", 1).append(err);

    log()->error("%s : %s", error_str.c_str(), err.c_str());
    return doc;
}

std::string
Block::errorLocation() const {
    std::stringstream stream;
    stream << "[Block: " << name();
    if (!method().empty()) {
        stream << ", method: " << method();
    }
    if (!id().empty()) {
        stream << ", id: " << id();
    }
    stream << "]";
    return stream.str();
}

std::string
Block::errorMessage(const InvokeError &error) const {
    std::stringstream stream;
    stream << error.whatStr() << " " << errorLocation();
    const InvokeError::InfoMapType& info = error.info();
    for (InvokeError::InfoMapType::const_iterator it = info.begin();
         it != info.end();
         ++it) {
        if (!it->second.empty()) {
            stream << ". " << it->first << ": " << it->second;
        }
    }
    stream << ". owner: " << owner()->name();
    return stream.str();
}

void
Block::throwBadArityError() const {
    throw CriticalInvokeError("bad arity");
}

bool
Block::checkGuard(Context *ctx) const {
    for(std::list<Guard>::iterator it = data_->guards_.begin();
        it != data_->guards_.end();
        ++it) {
        if (!it->check(ctx)) {
            return false;
        }
    }
    return true;
}

bool
Block::checkStateGuard(Context *ctx) const {
    for(std::list<Guard>::iterator it = data_->guards_.begin();
        it != data_->guards_.end();
        ++it) {
        if (it->isState() && !it->check(ctx)) {
            return false;
        }
    }
    return true;
}

bool
Block::hasGuard() const {
    return !data_->guards_.empty();
}

bool
Block::hasStateGuard() const {
    for(std::list<Guard>::iterator it = data_->guards_.begin();
        it != data_->guards_.end();
        ++it) {
        if (it->isState()) {
            return true;
        }
    }
    return false;
}

void
Block::evalXPath(Context *ctx, InvokeContext *invoke_ctx, const XmlDocSharedHelper &doc) const {
    if (ctx->noXsltPort() && !invoke_ctx->xsltName().empty()) {
        return;
    }

    XmlXPathContextHelper xctx(xmlXPathNewContext(doc.get()));
    XmlUtils::throwUnless(NULL != xctx.get());
    State *state = ctx->state();
    assert(NULL != state);

    for(std::list<XPathNodeExpr>::const_iterator iter = data_->xpath_.begin(), end = data_->xpath_.end();
        iter != end;
        ++iter) {

        std::string expr = iter->expression(ctx);
        if (expr.empty()) {
            continue;
        }
        
        XmlUtils::regiserNsList(xctx.get(), data_->namespaces_);
        XmlUtils::regiserNsList(xctx.get(), iter->namespaces());

        XmlXPathObjectHelper object = iter->eval(xctx.get(), ctx);

        if (XPATH_BOOLEAN == object->type) {
                state->checkName(iter->result());
                state->setBool(iter->result(), 0 != object->boolval);
        }
        else if (XPATH_NUMBER == object->type) {
                state->checkName(iter->result());
                state->setDouble(iter->result(), object->floatval);
        }
        else if (XPATH_STRING == object->type) {
                state->checkName(iter->result());
                state->setString(iter->result(), (const char *)object->stringval);
        }
        else if(NULL != object->nodesetval && 0 != object->nodesetval->nodeNr) {
            std::string val;
            for (int i = 0; i < object->nodesetval->nodeNr; ++i) {
                xmlNodePtr node = object->nodesetval->nodeTab[i];
                appendNodeValue(node, val);
                if (object->nodesetval->nodeNr - 1 != i) {
                    val.append(iter->delimiter());
                }
            }

            if (!val.empty()) {
                state->checkName(iter->result());
                state->setString(iter->result(), val);
            }
        }
    }
}

void
Block::appendNodeValue(xmlNodePtr node, std::string &val) const {

    const char *nodeval = "";
    if (XML_ELEMENT_NODE == node->type) {
        nodeval = XmlUtils::value(node);
        if (NULL == nodeval) {
            xmlNodePtr child = node->children;
            if (child && XML_CDATA_SECTION_NODE == child->type) {
                nodeval = (const char*)child->content;
            }
        }
    }
    else if (XML_ATTRIBUTE_NODE == node->type) {
        nodeval = XmlUtils::value((xmlAttrPtr) node);
    }
    else if (XML_TEXT_NODE == node->type && node->content) {
        nodeval = (const char*) node->content;
    }
    if (nodeval && strlen(nodeval)) {
        val.append(nodeval);
    }
}

XmlDocHelper
Block::fakeDoc() const {

    log()->debug("%s", BOOST_CURRENT_FUNCTION);
    XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != doc.get());
    return doc;
}

void
Block::postParse() {
    if (data_->meta_block_.get() && !tagged() && data_->meta_block_->haveCachedLua()) {
        throw std::runtime_error(
            "Meta cache lua sections are not allowed in non-tagged blocks");
    }
}

void
Block::property(const char *name, const char *value) {
    data_->property(name, value);
}

void
Block::postCall(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) {
    (void)ctx;
    (void)invoke_ctx;
}

ArgList*
Block::createArgList(Context *ctx, InvokeContext *invoke_ctx) const {
    (void)ctx;
    (void)invoke_ctx;
    return new CommonArgList();
}

void
Block::processArguments(Context *ctx, InvokeContext *invoke_ctx) const {
    boost::shared_ptr<ArgList> args(createArgList(ctx, invoke_ctx));
    const std::vector<Param*>& prms = params();
    for (std::vector<Param*>::const_iterator it = prms.begin();
         it != prms.end();
         ++it) {
        (*it)->add(ctx, *args);
    }
    invoke_ctx->setArgList(args);

    const std::vector<Param*>& params = xsltParams();
    for (std::vector<Param*>::const_iterator it = params.begin(), end = params.end();
        it != end;
        ++it) {
        invoke_ctx->appendXsltParam((*it)->asString(ctx));
    }
}

void
Block::postInvoke(Context *ctx, InvokeContext *invoke_ctx) {
    bool is_meta = isMeta();
    evalXPath(ctx, invoke_ctx, is_meta ? invoke_ctx->metaDoc() : invoke_ctx->resultDoc());
    boost::shared_ptr<XPathExpr> xpointer;
    if (!ctx->noXsltPort()) {
        if (data_->xpointer_expr_->compiled()) {
            xpointer = data_->xpointer_expr_;
        }
        else {
            std::string expr;
            if (!data_->xpointer_expr_->value().empty()) {
                expr = data_->xpointer_expr_->expression(ctx);
            }
            if (!expr.empty()) {
                xpointer.reset(new XPathExpr(expr.c_str(), NULL));
            }
        }
    }
    if (is_meta) {
        invoke_ctx->setMetaXPointer(xpointer);
    }
    else {
        invoke_ctx->setXPointer(xpointer);
    }
}

void
Block::callInternal(boost::shared_ptr<Context> ctx, unsigned int slot) {
    (void)ctx;
    (void)slot;
    assert(false);
}

void
Block::callInternal_Ex(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx, unsigned int slot) const {
    try {
        ctx->result(slot, invoke_Ex(ctx, invoke_ctx));
    }
    catch(const std::exception &e) {
        log()->error("exception caught in block call: %s", e.what());
    }
    catch(...) {
        log()->error("unknown exception caught in block call");
    }
}

void
Block::callInternalThreaded(boost::shared_ptr<Context> ctx, unsigned int slot) {
    (void)ctx;
    (void)slot;
    assert(false);
}

void
Block::callInternalThreaded_Ex(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx, unsigned int slot) const {
    XmlUtils::registerReporters();
    VirtualHostData::instance()->set(ctx->rootContext()->request());
    Context::resetTimer();
    callInternal_Ex(ctx, invoke_ctx, slot);
}

void
Block::callMetaLua(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) {
    if (NULL == data_->meta_block_.get()) {
        return;
    }
    try {
        data_->meta_block_->callLua(ctx, invoke_ctx);
    }
    catch (const InvokeError &e) {
        throw MetaInvokeError(e);
    }
    catch (const std::exception &e) {
        throw MetaInvokeError(e.what());
    }
}

void
Block::callMeta(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) {
    if (NULL == data_->meta_block_.get()) {
        return;
    }
    try {
        data_->meta_block_->call(ctx, invoke_ctx);
        data_->meta_block_->postInvoke(ctx.get(), invoke_ctx.get());
    }
    catch (const InvokeError &e) {
        throw MetaInvokeError(e);
    }
    catch (const std::exception &e) {
        throw MetaInvokeError(e.what());
    }
}

bool
Block::isMeta() const {
    return NULL != dynamic_cast<const MetaBlock*>(this);
}

MetaBlock*
Block::metaBlock() const {
    return data_->meta_block_.get();
}

bool
Block::metaNode(const xmlNodePtr node) const {
    return node->ns && node->ns->href &&
           xmlStrcasecmp(node->name, (const xmlChar*)"meta") == 0 &&
           xmlStrcasecmp(node->ns->href, (const xmlChar*)XmlUtils::XSCRIPT_NAMESPACE) == 0;
}

bool
Block::xpathNode(const xmlNodePtr node) const {
    return (xmlStrncasecmp(node->name, (const xmlChar*) "xpath", sizeof("xpath")) == 0);
}

bool
Block::xpointerNode(const xmlNodePtr node) const {
    return (xmlStrncasecmp(node->name, (const xmlChar*) "xpointer", sizeof("xpointer")) == 0);
}

bool
Block::paramNode(const xmlNodePtr node) const {
    return (xmlStrncasecmp(node->name, (const xmlChar*) "param", sizeof("param")) == 0);
}

bool
Block::guardNode(const xmlNodePtr node) const {
    return xmlStrncasecmp(node->name, (const xmlChar*) "guard", sizeof("guard")) == 0;
}

bool
Block::guardNotNode(const xmlNodePtr node) const {
    return xmlStrncasecmp(node->name, (const xmlChar*) "guard-not", sizeof("guard-not")) == 0;
}

bool
Block::xsltNode(const xmlNodePtr node) const {
    return xmlStrncasecmp(node->name, (const xmlChar*) "xslt", sizeof("xslt")) == 0;
}

void
Block::parseXPathNode(const xmlNodePtr node) {
    const char *expr = XmlUtils::attrValue(node, "expr");
    const char *result = XmlUtils::attrValue(node, "result");
    if (expr && *expr && result && *result) {
        const char *delim = XmlUtils::attrValue(node, "delim");
        if (!delim) {
            delim = " ";
        }
        const char *type = XmlUtils::attrValue(node, "type");
        data_->xpath_.push_back(XPathNodeExpr(expr, result, delim, type));
        data_->xpath_.back().compile();
        xmlNs* ns = node->nsDef;
        while (ns) {
            data_->xpath_.back().addNamespace((const char*)ns->prefix, (const char*)ns->href);
            ns = ns->next;
        }
    }
}

void
Block::parseXPointerNode(const xmlNodePtr node) {
    if (!data_->xpointer_expr_->value().empty()) {
        throw std::runtime_error("In block only one xpointer allowed");
    }
    parseXPointerExpr(XmlUtils::value(node), XmlUtils::attrValue(node, "type"));
}

void
Block::parseMetaNode(const xmlNodePtr node) {
    if (data_->meta_block_.get()) {
        throw std::runtime_error("One meta section allowed only");
    }
    data_->meta_block_ = std::auto_ptr<MetaBlock>(new MetaBlock(this, node));
    data_->meta_block_->parse();

}

void
Block::parseXPointerExpr(const char *value, const char *type) {
    disableOutput(false);
    if (NULL == value) {
        return;
    }
    std::string xpointer;
    if (strncasecmp(value, "xpointer(", sizeof("xpointer(") - 1) == 0) {
        xpointer.assign(value, sizeof("xpointer(") - 1, strlen(value) - sizeof("xpointer("));
    }
    else {
        xpointer.assign(value);
    }

    if (!data_->xpointer_expr_->assign(xpointer.c_str(), type)) {
        throw std::runtime_error("StateArg type in xpointer node allowed only");
    }

    if (data_->xpointer_expr_->stripAll()) {
        disableOutput(true);
    }
    else {
        data_->xpointer_expr_->compile();
    }
}

void
Block::disableOutput(bool flag) {
    data_->disable_output_ = flag;
}

bool
Block::disableOutput() const {
    return data_->disable_output_;
}

void
Block::parseGuardNode(const xmlNodePtr node, bool is_not) {
    const char *guard = XmlUtils::value(node);
    const char *type = XmlUtils::attrValue(node, "type");
    const char *value = XmlUtils::attrValue(node, "value");
    data_->guards_.push_back(Guard(guard, type, value, is_not));
}

void
Block::parseXsltNode(const xmlNodePtr node) {
    const char *xslt = XmlUtils::value(node);
    const char *type = XmlUtils::attrValue(node, "type");
    xsltName(xslt, type);
}


void
Block::parseParamNode(const xmlNodePtr node) {
    std::auto_ptr<Param> p = createParam(node);
    addParam(p);
}

void
Block::addParam(std::auto_ptr<Param> param) {
    data_->params_.push_back(param.get());
    param.release();
}

Logger*
Block::log() const {
    return data_->extension_->getLogger();
}

void
Block::startTimer(Context *ctx) {
    ctx->startTimer(TimeoutCounter::UNLIMITED_TIME);
}

void
Block::stopTimer(Context *ctx) {
    ctx->stopTimer();
}

std::string
Block::concatArguments(const ArgList *args, unsigned int first, unsigned int last) {
    unsigned int size = args->size();
    if (first >= size) {
        return StringUtils::EMPTY_STRING;
    }
    last = std::min(last, size - 1);
    std::string result;
    for (unsigned int i = first; i <= last; ++i) {
        result.append(args->at(i));
    }
    return result;
}

} // namespace xscript
