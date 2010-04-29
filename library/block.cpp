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

#include "internal/param_factory.h"
#include "xscript/block.h"
#include "xscript/context.h"
#include "xscript/guard_checker.h"
#include "xscript/logger.h"
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

const static std::map<std::string, std::string> EMPTY_MAP;

class XPathExpr {
public:
    XPathExpr(const char* expression, const char* result, const char* delimeter, const char* type);
    ~XPathExpr();

    std::string expression(Context *ctx) const;
    const std::string& result() const;
    const std::string& delimeter() const;
    const std::map<std::string, std::string>& namespaces() const;
    void addNamespace(const char* prefix, const char* uri);

private:
    std::string expression_;
    std::string result_;
    std::string delimeter_;
    std::map<std::string, std::string> namespaces_;
    bool from_state_;
};

class Guard {
public:
    Guard(const char *expr, const char *type, const char *value, bool is_not);
    bool check(const Context *ctx);
    bool isState() const;

private:
    std::string guard_;
    std::string value_;
    bool not_;
    bool state_;
    GuardCheckerMethod method_;
    static const std::string STATE_ARG_PARAM_NAME;
};

const std::string Guard::STATE_ARG_PARAM_NAME = "StateArg";

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
    std::vector<XPathExpr> xpath_;
    std::vector<Guard> guards_;
    std::string id_, method_;
    std::string xpointer_expr_;
    bool xpointer_from_state_;
    std::string base_;
    std::map<std::string, std::string> namespaces_;
    bool disable_output_;
    
    static const std::string XSCRIPT_INVOKE_FAILED;
    static const std::string XSCRIPT_INVOKE_INFO;
};

const std::string Block::BlockData::XSCRIPT_INVOKE_FAILED = "xscript_invoke_failed";
const std::string Block::BlockData::XSCRIPT_INVOKE_INFO = "xscript_invoke_info";

Block::BlockData::BlockData(const Extension *ext, Xml *owner, xmlNodePtr node, Block *block) :
    extension_(ext), owner_(owner), node_(node), block_(block),
    xpointer_from_state_(false), disable_output_(false)
{}

Block::BlockData::~BlockData() {
    std::for_each(params_.begin(), params_.end(), boost::checked_deleter<Param>());
}

void
Block::BlockData::parseNamespaces() {
    xmlNs* ns = node_->nsDef;
    while(ns) {
        if (ns->prefix && ns->href) {
            namespaces_.insert(std::make_pair(std::string((const char*)ns->prefix),
                                              std::string((const char*)ns->href)));
        }
        ns = ns->next;
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
        block_->xsltName(value);
    }
    else if (strncasecmp(name, "xpointer", sizeof("xpointer")) == 0) {
        block_->parseXPointerExpr(value);
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
    delete data_;
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

const std::string&
Block::xpointerExpr() const {
    return data_->xpointer_expr_;
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
            std::stringstream stream;
            stream << "param with id not found: " << id;
            throw std::invalid_argument(stream.str());
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

XmlXPathObjectHelper
Block::evalXPathExpression(const std::string &expr, xmlXPathContextPtr context,
        const std::map<std::string, std::string> &ext_namespaces) const {
    try {
        const std::map<std::string, std::string>& block_nslist = data_->namespaces_;
        for(std::map<std::string, std::string>::const_iterator it_ns = block_nslist.begin();
            it_ns != block_nslist.end();
            ++it_ns) {
            xmlXPathRegisterNs(context,
                               (const xmlChar *)it_ns->first.c_str(),
                               (const xmlChar *)it_ns->second.c_str());
        }

        for(std::map<std::string, std::string>::const_iterator it_ns = ext_namespaces.begin();
            it_ns != ext_namespaces.end();
            ++it_ns) {
            xmlXPathRegisterNs(context,
                               (const xmlChar *)it_ns->first.c_str(),
                               (const xmlChar *)it_ns->second.c_str());
        }

        XmlXPathObjectHelper xpathObj(xmlXPathEvalExpression((const xmlChar *)expr.c_str(), context));
        XmlUtils::throwUnless(NULL != xpathObj.get());
        return xpathObj;
    }
    catch(const std::exception &e) {
        std::string message = "XPath error with expression " + expr + " : ";
        message.append(e.what());
        throw std::runtime_error(message);
    }
}

bool
Block::processXPointer(const Context *ctx, xmlDocPtr doc, xmlNodePtr insert_node, bool replace) const {
    if (ctx->noXsltPort() || data_->xpointer_expr_.empty()) {
        return false;
    }

    std::string expr = data_->xpointer_from_state_ ?
            ctx->state()->asString(data_->xpointer_expr_, StringUtils::EMPTY_STRING) :
            data_->xpointer_expr_;

    if (expr.empty()) {
        return false;
    }

    if (data_->disable_output_ || "/.." == expr) {
        if (replace) {
            xmlUnlinkNode(insert_node);
        }
        return true;
    }

    XmlXPathContextHelper context(xmlXPathNewContext(doc));
    XmlUtils::throwUnless(NULL != context.get());

    XmlXPathObjectHelper xpathObj = evalXPathExpression(expr, context.get(), EMPTY_MAP);
    if (XPATH_BOOLEAN == xpathObj->type) {
        const char *str = 0 != xpathObj->boolval ? "1" : "0";
        xmlNodePtr text_node = xmlNewText((const xmlChar *)str);
        replace ? xmlReplaceNode(insert_node, text_node) : xmlAddChild(insert_node, text_node);
        return true;
    }
    if (XPATH_NUMBER == xpathObj->type) {
        char str[40];
        snprintf(str, sizeof(str) - 1, "%f", xpathObj->floatval);
        str[sizeof(str) - 1] = '\0';
        xmlNodePtr text_node = xmlNewText((const xmlChar *)&str[0]);
        replace ? xmlReplaceNode(insert_node, text_node) : xmlAddChild(insert_node, text_node);
        return true;
    }
    if (XPATH_STRING == xpathObj->type) {
        xmlNodePtr text_node = xmlNewText((const xmlChar *)xpathObj->stringval);
        replace ? xmlReplaceNode(insert_node, text_node) : xmlAddChild(insert_node, text_node);
        return true;
    }

    xmlNodeSetPtr nodeset = xpathObj->nodesetval;
    if (NULL == nodeset || 0 == nodeset->nodeNr) {
        if (replace) {
            xmlUnlinkNode(insert_node);
        }
        return true;
    }

    xmlNodePtr current_node = nodeset->nodeTab[0];
    if (XML_ATTRIBUTE_NODE == current_node->type) {
        current_node = current_node->children;
    }
    xmlNodePtr last_input_node = xmlCopyNode(current_node, 1);
    replace ? xmlReplaceNode(insert_node, last_input_node) :
        xmlAddChild(insert_node, last_input_node);

    for (int i = 1; i < nodeset->nodeNr; ++i) {
        xmlNodePtr current_node = nodeset->nodeTab[i];
        if (XML_ATTRIBUTE_NODE == current_node->type) {
            current_node = current_node->children;
        }
        xmlNodePtr insert_node = xmlCopyNode(current_node, 1);
        xmlAddNextSibling(last_input_node, insert_node);
        last_input_node = insert_node;
    }

    return true;
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

const std::map<std::string, std::string>&
Block::namespaces() const {
    return data_->namespaces_;
}

void
Block::parse() {
    try {       
        detectBase();
        
        XmlUtils::visitAttributes(data_->node_->properties,
            boost::bind(&Block::property, this, _1, _2));

        data_->parseNamespaces();
        
        for (xmlNodePtr node = data_->node_->children; NULL != node; node = node->next) {
            parseSubNode(node);
        }
        postParse();
    }
    catch(const std::exception &e) {
        log()->error("%s, parse failed for '%s': %s", name(), owner()->name().c_str(), e.what());
        throw;
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
    log()->debug("%s", BOOST_CURRENT_FUNCTION);

    if (ctx->stopped()) {
        log()->error("Context already stopped. Cannot invoke block. Owner: %s. Block: %s. Method: %s",
            owner()->name().c_str(), name(), data_->method_.c_str());
        return fakeResult(true);
    }

    boost::shared_ptr<InvokeContext> result;
    try {
        if (!checkGuard(ctx.get())) {
            log()->info("Guard skipped block processing. Owner: %s. Block: %s. Method: %s",
                owner()->name().c_str(), name(), data_->method_.c_str());
            return fakeResult(false);
        }
        BlockTimerStarter starter(ctx.get(), this);
        result = boost::shared_ptr<InvokeContext>(new InvokeContext());
        invokeInternal(ctx, result);
        if (!result->error()) {
            postInvoke(ctx.get(), result.get());
        }
    }
    catch (const CriticalInvokeError &e) {
        std::string full_error;
        result = errorResult(e, full_error);        
        OperationMode::assignBlockError(ctx.get(), this, full_error);
    }
    catch (const SkipResultInvokeError &e) {
        log()->info("%s", errorMessage(e).c_str());
        result = errorResult(fakeDoc());
    }
    catch (const InvokeError &e) {
        result = errorResult(e, false);
    }
    catch (const std::exception &e) {
        result = errorResult(e.what(), false);
    }
    
    if (!result->success()) {
        ctx->setNoCache();
    }
    
    return result;
}

void
Block::invokeInternal(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) {
    log()->debug("%s", BOOST_CURRENT_FUNCTION);

    // Check validators for each param before calling it.
    for(std::vector<Param*>::const_iterator i = data_->params_.begin();
        i != data_->params_.end();
        ++i) {
        (*i)->checkValidator(ctx.get());
    }
    
    XmlDocHelper doc(call(ctx, invoke_ctx));
    if (NULL == doc.get()) {
        errorResult("got empty document", false, invoke_ctx);
        return;
    }
    invoke_ctx->resultDoc(doc);
    processResponse(ctx, invoke_ctx);
}

void
Block::invokeCheckThreaded(boost::shared_ptr<Context> ctx, unsigned int slot) {
    if (threaded() && !ctx->forceNoThreaded()) {
        boost::function<void()> f = boost::bind(&Block::callInternalThreaded, this, ctx, slot);
        ThreadPool::instance()->invoke(f);
    }
    else {
        callInternal(ctx, slot);
    }
}

void
Block::processResponse(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) { 
    XmlDocSharedHelper doc = invoke_ctx->resultDoc();
    if (NULL == doc->get()) {
        throw InvokeError("null response document");
    }

    if (NULL == xmlDocGetRootElement(doc->get())) {
        throw InvokeError("got document with no root");
    }

    if (!tagged() && ctx->stopped()) {
        throw InvokeError("context is already stopped, cannot process response");
    }

    bool is_error_doc = Policy::isErrorDoc(doc->get());
    
    log()->debug("%s, got source document: %p", BOOST_CURRENT_FUNCTION, doc->get());
    
    bool need_perblock = !ctx->noXsltPort() && !xsltName().empty();
    bool success = true;
    if (need_perblock) {
        boost::shared_ptr<Context> local_ctx = invoke_ctx->getLocalContext();
        success = applyStylesheet(local_ctx.get() ? local_ctx: ctx, doc);
    }

    InvokeContext::ResultType type = InvokeContext::SUCCESS;    
    if (is_error_doc || !success || invoke_ctx->noCache()) {
        type = InvokeContext::NO_CACHE;
    }
    
    invoke_ctx->resultType(type);
    invoke_ctx->resultDoc(doc);
    
    postCall(ctx.get(), invoke_ctx.get());

    if (need_perblock || xsltName().empty()) {
        evalXPath(ctx.get(), doc);
    }
}

bool
Block::applyStylesheet(boost::shared_ptr<Context> ctx, XmlDocSharedHelper &doc) {

    const TimeoutCounter &timer = ctx->timer();
    if (timer.expired()) {
        throw InvokeError("block is timed out", "timeout",
            boost::lexical_cast<std::string>(timer.timeout()));
    }
    boost::shared_ptr<Stylesheet> sh = StylesheetFactory::createStylesheet(xsltName());
    {
        PROFILER(log(), std::string("per-block-xslt: '") + xsltName() +
                "' block: '" + name() + "' block-id: '" + id() +
                "' method: '" + method() + "' owner: '" + owner()->name() + "'");
        Object::applyStylesheet(sh, ctx, doc, true);
    }

    XmlUtils::throwUnless(NULL != doc->get());
    log()->debug("%s, got source document: %p", BOOST_CURRENT_FUNCTION, doc->get());
        
    bool result = true;
    if (XmlUtils::hasXMLError()) {
        result = false;
    }
                
    OperationMode::processPerblockXsltError(ctx.get(), this);
                    
    return result;
}

void
Block::errorResult(const char *error, bool info, boost::shared_ptr<InvokeContext> &ctx) const {
    std::string full_error;
    InvokeError invoke_error(error);
    XmlDocHelper doc = errorDoc(invoke_error, info ? BlockData::XSCRIPT_INVOKE_INFO.c_str() :
        BlockData::XSCRIPT_INVOKE_FAILED.c_str(), full_error);
    
    ctx->resultDoc(doc);
    ctx->resultType(InvokeContext::ERROR);
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

boost::shared_ptr<InvokeContext>
Block::errorResult(const InvokeError &error, std::string &full_error) const {
    return errorResult(errorDoc(error, BlockData::XSCRIPT_INVOKE_FAILED.c_str(), full_error));
}

boost::shared_ptr<InvokeContext>
Block::errorResult(XmlDocHelper doc) const {
    boost::shared_ptr<InvokeContext> ctx(new InvokeContext());
    ctx->resultDoc(doc);
    ctx->resultType(InvokeContext::ERROR);
    return ctx;
}

boost::shared_ptr<InvokeContext>
Block::fakeResult(bool error) const {   
    boost::shared_ptr<InvokeContext> ctx(new InvokeContext());
    ctx->resultDoc(fakeDoc());
    ctx->resultType(error ? InvokeContext::ERROR : InvokeContext::SUCCESS);
    return ctx;
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
    stream << error.what() << ". ";
    
    xmlNewProp(main_node.get(), (const xmlChar*)"block", (const xmlChar*)name());
    stream << "block: " << name() << ". ";
    
    if (!method().empty()) {
        xmlNewProp(main_node.get(), (const xmlChar*)"method", (const xmlChar*)method().c_str());
        stream << "method: " << method() << ". ";
    }

    if (!id().empty()) {
        xmlNewProp(main_node.get(), (const xmlChar*)"id", (const xmlChar*)id().c_str());
        stream << "id: " << id() << ". ";
    }

    const InvokeError::InfoMapType& info = error.info();
    for(InvokeError::InfoMapType::const_iterator it = info.begin();
        it != info.end();
        ++it) {
        if (!it->second.empty()) {
            xmlNewProp(main_node.get(), (const xmlChar*)it->first.c_str(), (const xmlChar*)it->second.c_str());
            stream << it->first << ": " << it->second << ". ";
        }
    }

    stream << "owner: " << owner()->name() << ".";

    XmlNodeHelper node = error.what_node();
    if (node.get()) {
        xmlAddChild(main_node.get(), node.release());
    }
    
    xmlDocSetRootElement(doc.get(), main_node.release());
    
    full_error.assign(stream.str());
    log()->error("%s", full_error.c_str());
    
    return doc;
}

std::string
Block::errorMessage(const InvokeError &error) const {

    std::stringstream stream;
    stream << error.what() << ". " << "block: " << name() << ". ";

    if (!method().empty()) {
        stream << "method: " << method() << ". ";
    }

    if (!id().empty()) {
        stream << "id: " << id() << ". ";
    }

    const InvokeError::InfoMapType& info = error.info();
    for(InvokeError::InfoMapType::const_iterator it = info.begin();
        it != info.end();
        ++it) {
        if (!it->second.empty()) {
            stream << it->first << ": " << it->second << ". ";
        }
    }

    stream << "owner: " << owner()->name() << ".";

    return stream.str();
}

void
Block::throwBadArityError() const {
    throw CriticalInvokeError("bad arity");
}

bool
Block::checkGuard(Context *ctx) const {
    for(std::vector<Guard>::iterator it = data_->guards_.begin();
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
    for(std::vector<Guard>::iterator it = data_->guards_.begin();
        it != data_->guards_.end();
        ++it) {
        if (it->isState() && !it->check(ctx)) {
            return false;
        }
    }
    return true;
}

bool
Block::hasStateGuard() const {
    for(std::vector<Guard>::iterator it = data_->guards_.begin();
        it != data_->guards_.end();
        ++it) {
        if (it->isState()) {
            return true;
        }
    }
    return false;
}

void
Block::evalXPath(Context *ctx, const XmlDocSharedHelper &doc) const {

    XmlXPathContextHelper xctx(xmlXPathNewContext(doc->get()));
    XmlUtils::throwUnless(NULL != xctx.get());
    State *state = ctx->state();
    assert(NULL != state);

    for(std::vector<XPathExpr>::const_iterator iter = data_->xpath_.begin(), end = data_->xpath_.end();
        iter != end;
        ++iter) {

        std::string expr = iter->expression(ctx);
        if (expr.empty()) {
            continue;
        }
        
        XmlXPathObjectHelper object = evalXPathExpression(expr, xctx.get(), iter->namespaces());

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
                    val.append(iter->delimeter());
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
}

void
Block::property(const char *name, const char *value) {
    data_->property(name, value);
}

void
Block::postCall(Context *, InvokeContext *) {
}

void
Block::postInvoke(Context *, InvokeContext *) {
}

void
Block::callInternal(boost::shared_ptr<Context> ctx, unsigned int slot) {
    try {
        ctx->result(slot, invoke(ctx));
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
    XmlUtils::registerReporters();
    VirtualHostData::instance()->set(ctx->rootContext()->request());
    Context::resetTimer();
    callInternal(ctx, slot);
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
        data_->xpath_.push_back(XPathExpr(expr, result, delim, type));
        xmlNs* ns = node->nsDef;
        while (ns) {
            data_->xpath_.back().addNamespace((const char*)ns->prefix, (const char*)ns->href);
            ns = ns->next;
        }
    }
}

void
Block::parseXPointerNode(const xmlNodePtr node) {

    if (!data_->xpointer_expr_.empty()) {
        throw std::runtime_error("In block only one xpointer allowed");
    }

    parseXPointerExpr(XmlUtils::value(node));
    const char *type = XmlUtils::attrValue(node, "type");
    if (NULL == type) {
        return;
    }
    if (strncasecmp(type, "statearg", sizeof("statearg")) != 0) {
        throw std::runtime_error("StateArg type in xpointer node allowed only");
    }
    data_->xpointer_from_state_ = true;
}

void
Block::parseXPointerExpr(const char *value) {
    if (NULL == value) {
        return;
    }
    if (strncasecmp(value, "xpointer(", sizeof("xpointer(") - 1) == 0) {
        data_->xpointer_expr_.assign(value, sizeof("xpointer(") - 1, strlen(value) - sizeof("xpointer("));
    }
    else {
        data_->xpointer_expr_.assign(value);
    }

    if ("/.." == data_->xpointer_expr_ && !data_->xpointer_from_state_) {
        data_->disable_output_ = true;
    }
}

void
Block::parseGuardNode(const xmlNodePtr node, bool is_not) {
    const char *guard = XmlUtils::value(node);
    const char *type = XmlUtils::attrValue(node, "type");
    const char *value = XmlUtils::attrValue(node, "value");
    data_->guards_.push_back(Guard(guard, type, value, is_not));
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
    (void)ctx;
}

void
Block::stopTimer(Context *ctx) {
    (void)ctx;
}

std::string
Block::concatParams(const Context *ctx, unsigned int first, unsigned int last) const {
    const std::vector<Param*> &p = params();
    unsigned int size = p.size();
    if (first >= size) {
        return StringUtils::EMPTY_STRING;
    }
    
    last = std::min(last, size - 1);
    
    std::string result;
    for(unsigned int i = first; i <= last; ++i) {
        result.append(p[i]->asString(ctx));
    }
    
    return result;
}

Guard::Guard(const char *expr, const char *type, const char *value, bool is_not) :
    guard_(expr ? expr : ""),
    value_(value ? value : ""),
    not_(is_not), state_(false),
    method_(GuardChecker::instance()->method(type ? type : STATE_ARG_PARAM_NAME))
{   
    if (NULL == method_) {
        std::stringstream stream;
        stream << "Incorrect guard type. Type: ";
        if (type) {
            stream << type;
        }
        stream << ". Guard: " << guard_;
        throw std::runtime_error(stream.str());
    }
    
    if (!GuardChecker::instance()->allowed(type ? type : STATE_ARG_PARAM_NAME.c_str(), guard_.empty())) {
        std::stringstream stream;
        stream << "Guard is not allowed. Type: ";
        if (type) {
            stream << type;
        }
        stream << ". Guard: " << guard_;
        throw std::runtime_error(stream.str());
    }

    if (NULL == type || 0 == strcasecmp(STATE_ARG_PARAM_NAME.c_str(), type)) {
        state_ = true;
    }
}

bool
Guard::check(const Context *ctx) {
    return not_ ^ (*method_)(ctx, guard_, value_);
}

bool
Guard::isState() const {
    return state_;
}

XPathExpr::XPathExpr(const char* expression, const char* result, const char* delimeter, const char* type) :
    expression_(expression ? expression : ""), result_(result ? result : ""),
    delimeter_(delimeter ? delimeter : ""), from_state_(false)
{
    if (type && strncasecmp(type, "statearg", sizeof("statearg")) == 0) {
        from_state_ = true;
    }
}

XPathExpr::~XPathExpr()
{}

std::string
XPathExpr::expression(Context *ctx) const {
    if (!from_state_) {
        return expression_;
    }
    return ctx->state()->asString(expression_, StringUtils::EMPTY_STRING);
}

const std::string&
XPathExpr::result() const {
    return result_;
}

const std::string&
XPathExpr::delimeter() const {
    return delimeter_;
}

const std::map<std::string, std::string>&
XPathExpr::namespaces() const {
    return namespaces_;
}

void
XPathExpr::addNamespace(const char* prefix, const char* uri) {
    if (prefix && uri) {
        namespaces_.insert(std::make_pair(std::string(prefix), std::string(uri)));
    }
}

} // namespace xscript
