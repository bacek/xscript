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

#include "xscript/xml.h"
#include "xscript/xml_util.h"
#include "xscript/state.h"
#include "xscript/param.h"
#include "xscript/block.h"
#include "xscript/logger.h"
#include "xscript/object.h"
#include "xscript/operation_mode.h"
#include "xscript/context.h"
#include "xscript/vhost_data.h"
#include "xscript/stylesheet.h"
#include "xscript/thread_pool.h"
#include "xscript/profiler.h"
#include "internal/param_factory.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

struct Block::BlockData {
    BlockData(const Extension *ext, Xml *owner, xmlNodePtr node) :
        extension_(ext), owner_(owner), node_(node), is_guard_not_(false)
    {}
    
    ~BlockData() {
        std::for_each(params_.begin(), params_.end(), boost::checked_deleter<Param>());
    }
    
    const Extension *extension_;
    Xml *owner_;
    xmlNodePtr node_;
    std::vector<Param*> params_;
    std::vector<XPathExpr> xpath_;
    std::string id_, guard_, method_;
    bool is_guard_not_;
    std::string xpointer_expr_;
};

Block::Block(const Extension *ext, Xml *owner, xmlNodePtr node) :
    data_(new BlockData(ext, owner, node))
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
Block::guard() const {
    return data_->guard_;
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

bool
Block::xpointer(Context* ctx) const {
    if (data_->xpointer_expr_.empty()) {
        return false;
    }
    return !ctx->noXsltPort();
}

void
Block::parse() {
    try {
        XmlUtils::visitAttributes(data_->node_->properties,
            boost::bind(&Block::property, this, _1, _2));

        ParamFactory *pf = ParamFactory::instance();
        for (xmlNodePtr node = data_->node_->children; NULL != node; node = node->next) {
            if (node->name) {
                if (xpathNode(node)) {
                    parseXPathNode(node);
                }
                else if (paramNode(node)) {
                    parseParamNode(node, pf);
                }
                else if (xsltParamNode(node)) {
                    parseXsltParamNode(node, pf);
                }
                else if (XML_ELEMENT_NODE == node->type) {
                    const char *value = XmlUtils::value(node);
                    if (value) {
                        property((const char*) node->name, value);
                    }
                }
            }
        }
        postParse();
    }
    catch(const std::exception &e) {
        log()->error("%s, parse failed for '%s': %s", name(), owner()->name().c_str(), e.what());;
        throw;
    }
}

std::string
Block::fullName(const std::string &name) const {
    return owner()->fullName(name);
}

InvokeResult
Block::invoke(boost::shared_ptr<Context> ctx) {
    log()->debug("%s", BOOST_CURRENT_FUNCTION);
    if (ctx->stopped()) {
        log()->error("Context already stopped. Cannot invoke block. Owner: %s. Block: %s. Method: %s",
            owner()->name().c_str(), name(), data_->method_.c_str());
        return InvokeResult(fakeResult(), false);
    }

    if (!checkGuard(ctx.get())) {
        log()->info("Guard skipped block processing. Owner: %s. Block: %s. Method: %s",
            owner()->name().c_str(), name(), data_->method_.c_str());
        return InvokeResult(fakeResult(), false);
    }

    InvokeResult result;
    try {
        BlockTimerStarter starter(ctx.get(), this);
        result = invokeInternal(ctx);
        if (result.success) {
            postInvoke(ctx.get(), result.doc);
        }
    }
    catch (const CriticalInvokeError &e) {
        std::string full_error;
        result = errorResult(e.what(), e.info(), full_error);        
        OperationMode::instance()->assignBlockError(ctx.get(), this, full_error);
    }
    catch (const SkipResultInvokeError &e) {
        log()->info("%s", errorMessage(e.what(), e.info()).c_str());
        result = InvokeResult(fakeResult(), false);
    }
    catch (const InvokeError &e) {
        result = errorResult(e.what(), e.info());
    }
    catch (const std::exception &e) {
        result = errorResult(e.what());
    }
    
    if (!result.success) {
        ctx->rootContext()->setError();
    }
    
    return result;
}

InvokeResult
Block::invokeInternal(boost::shared_ptr<Context> ctx) {
    log()->debug("%s", BOOST_CURRENT_FUNCTION);

    // Check validators for each param before calling it.
    for(std::vector<Param*>::const_iterator i = data_->params_.begin();
        i != data_->params_.end();
        ++i) {
        (*i)->checkValidator(ctx.get());
    }

    boost::any a;
    XmlDocHelper doc(call(ctx, a));

    if (NULL == doc.get()) {
        return errorResult("got empty document");
    }

    return InvokeResult(processResponse(ctx, doc, a), true);
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

XmlDocHelper
Block::processResponse(boost::shared_ptr<Context> ctx, XmlDocHelper doc, boost::any &a) {
    if (NULL == doc.get()) {
        throw InvokeError("null response document");
    }

    if (NULL == xmlDocGetRootElement(doc.get())) {
        log()->error("%s, got document with no root", BOOST_CURRENT_FUNCTION);
        throw InvokeError("got document with no root");
    }

    if (!tagged() && ctx->stopped()) {
        throw InvokeError("context is already stopped, cannot process response");
    }

    log()->debug("%s, got source document: %p", BOOST_CURRENT_FUNCTION, doc.get());
    bool need_perblock = !ctx->noXsltPort();
    if (need_perblock) {
        applyStylesheet(ctx, doc);
    }

    postCall(ctx.get(), doc, a);

    if (need_perblock || xsltName().empty()) {
        evalXPath(ctx.get(), doc);
    }

    return doc;
}

void
Block::applyStylesheet(boost::shared_ptr<Context> ctx, XmlDocHelper &doc) {
    if (!xsltName().empty()) {
        const TimeoutCounter &timer = ctx->timer();
        if (timer.expired()) {
            throw InvokeError("block is timed out", "timeout",
                boost::lexical_cast<std::string>(timer.timeout()));
        }
        boost::shared_ptr<Stylesheet> sh = Stylesheet::create(xsltName());
        {
            PROFILER(log(), std::string("per-block-xslt: '") + xsltName() +
                    "' block: '" + name() + "' block-id: '" + id() +
                    "' method: '" + method() + "' owner: '" + owner()->name() + "'");
            Object::applyStylesheet(sh, ctx, doc, true);
        }

        XmlUtils::throwUnless(NULL != doc.get());
        log()->debug("%s, got source document: %p", BOOST_CURRENT_FUNCTION, doc.get());

        OperationMode::instance()->processPerblockXsltError(ctx.get(), this);
    }
}

InvokeResult
Block::errorResult(const char *error, const InvokeError::InfoMapType &error_info) const {
    std::string full_error;
    return errorResult(error, error_info, full_error);
}

InvokeResult
Block::errorResult(const char *error, const InvokeError::InfoMapType &error_info, std::string &full_error) const {

    XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != doc.get());

    XmlNodeHelper node(xmlNewDocNode(doc.get(), NULL, (const xmlChar*)"xscript_invoke_failed", NULL));
    XmlUtils::throwUnless(NULL != node.get());

    std::stringstream stream;
    stream << "Caught invocation error" << ": ";
    if (error != NULL) {
        xmlNewProp(node.get(), (const xmlChar*)"error", (const xmlChar*)error);
        stream << error << ". ";
    }

    xmlNewProp(node.get(), (const xmlChar*)"block", (const xmlChar*)name());
    stream << "block: " << name() << ". ";

    if (!method().empty()) {
        xmlNewProp(node.get(), (const xmlChar*)"method", (const xmlChar*)method().c_str());
        stream << "method: " << method() << ". ";
    }

    if (!id().empty()) {
        xmlNewProp(node.get(), (const xmlChar*)"id", (const xmlChar*)id().c_str());
        stream << "id: " << id() << ". ";
    }

    for(InvokeError::InfoMapType::const_iterator it = error_info.begin();
        it != error_info.end();
        ++it) {
        if (!it->second.empty()) {
            xmlNewProp(node.get(), (const xmlChar*)it->first.c_str(), (const xmlChar*)it->second.c_str());
            stream << it->first << ": " << it->second << ". ";
        }
    }

    stream << "owner: " << owner()->name() << ".";

    xmlDocSetRootElement(doc.get(), node.release());

    full_error.assign(stream.str());
    log()->error("%s", full_error.c_str());

    return InvokeResult(doc, false);
}

std::string
Block::errorMessage(const char *error, const InvokeError::InfoMapType &error_info) const {

    std::stringstream stream;
    stream << "Caught invocation error" << ": ";
    if (error != NULL) {
        stream << error << ". ";
    }

    stream << "block: " << name() << ". ";

    if (!method().empty()) {
        stream << "method: " << method() << ". ";
    }

    if (!id().empty()) {
        stream << "id: " << id() << ". ";
    }

    for(InvokeError::InfoMapType::const_iterator it = error_info.begin();
        it != error_info.end();
        ++it) {
        if (!it->second.empty()) {
            stream << it->first << ": " << it->second << ". ";
        }
    }

    stream << "owner: " << owner()->name() << ".";

    return stream.str();
}

InvokeResult
Block::errorResult(const char *error) const {
    return errorResult(error, InvokeError::InfoMapType());
}

void
Block::throwBadArityError() const {
    throw CriticalInvokeError("bad arity");
}

bool
Block::checkGuard(Context *ctx) const {
    if (!data_->guard_.empty()) {
        return data_->is_guard_not_ ^ ctx->state()->is(data_->guard_);
    }
    return true;
}

void
Block::evalXPath(Context *ctx, const XmlDocHelper &doc) const {

    XmlXPathContextHelper xctx(xmlXPathNewContext(doc.get()));
    XmlUtils::throwUnless(NULL != xctx.get());

    for(std::vector<XPathExpr>::const_iterator iter = data_->xpath_.begin(), end = data_->xpath_.end();
        iter != end;
        ++iter) {
        
        const XPathExpr::NamespaceListType& ns_list = iter->namespaces();
        for (XPathExpr::NamespaceListType::const_iterator it_ns = ns_list.begin(); it_ns != ns_list.end(); ++it_ns) {
            xmlXPathRegisterNs(xctx.get(), (const xmlChar *)it_ns->first.c_str(), (const xmlChar *)it_ns->second.c_str());
        }
        XmlXPathObjectHelper object(xmlXPathEvalExpression((const xmlChar*)iter->expression().c_str(), xctx.get()));
        XmlUtils::throwUnless(NULL != object.get());

        if (NULL != object->nodesetval && 0 != object->nodesetval->nodeNr) {
            std::string val;
            for (int i = 0; i < object->nodesetval->nodeNr; ++i) {
                xmlNodePtr node = object->nodesetval->nodeTab[i];
                appendNodeValue(node, val);
                if (object->nodesetval->nodeNr - 1 != i) {
                    val.append(iter->delimeter());
                }
            }

            if (!val.empty()) {
                State* state = ctx->state();
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
    }
    else if (XML_ATTRIBUTE_NODE == node->type) {
        nodeval = XmlUtils::value((xmlAttrPtr) node);
    }
    else if (XML_TEXT_NODE ==node->type && node->content) {
        nodeval = (const char*) node->content;
    }
    if (nodeval && strlen(nodeval)) {
        val.append(nodeval);
    }
}

XmlDocHelper
Block::fakeResult() const {

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

    log()->debug("setting %s=%s", name, value);

    if (strncasecmp(name, "id", sizeof("id")) == 0) {
        if (!data_->id_.empty()) {
            throw std::runtime_error("duplicated id in block");
        }
        data_->id_.assign(value);
    }
    else if (strncasecmp(name, "guard", sizeof("guard")) == 0) {
        if (!data_->guard_.empty()) {
            throw std::runtime_error("duplicated guard in block");
        }
        if (*value == '\0') {
            throw std::runtime_error("empty guard");
        }
        data_->guard_.assign(value);
    }
    else if (strncasecmp(name, "guard-not", sizeof("guard-not")) == 0) {
        if (!data_->guard_.empty()) {
            throw std::runtime_error("duplicated guard in block");
        }
        if (*value == '\0') {
            throw std::runtime_error("empty guard-not");
        }
        data_->guard_.assign(value);
        data_->is_guard_not_ = true;
    }
    else if (strncasecmp(name, "method", sizeof("method")) == 0) {
        if (!data_->method_.empty()) {
            throw std::runtime_error("duplicated method in block");
        }
        data_->method_.assign(value);
    }
    else if (strncasecmp(name, "xslt", sizeof("xslt")) == 0) {
        xsltName(value);
    }
    else if (strncasecmp(name, "xpointer", sizeof("xpointer")) == 0) {
        if (strncasecmp(value, "xpointer(", sizeof("xpointer(") - 1) == 0) {
            data_->xpointer_expr_.assign(value, sizeof("xpointer(") - 1, strlen(value) - sizeof("xpointer("));
        }
        else {
            data_->xpointer_expr_.assign(value);
        }
    }
    else {
        std::stringstream stream;
        stream << "bad block attribute: " << name;
        throw std::invalid_argument(stream.str());
    }
}

void
Block::postCall(Context *, const XmlDocHelper &, const boost::any &) {
}

void
Block::postInvoke(Context *, const XmlDocHelper &) {
}

void
Block::callInternal(boost::shared_ptr<Context> ctx, unsigned int slot) {
    ctx->result(slot, invoke(ctx));
}

void
Block::callInternalThreaded(boost::shared_ptr<Context> ctx, unsigned int slot) {
    XmlUtils::registerReporters();
    VirtualHostData::instance()->set(ctx->request());
    Context::resetTimer();
    callInternal(ctx, slot);
}

bool
Block::xpathNode(const xmlNodePtr node) const {
    return (xmlStrncasecmp(node->name, (const xmlChar*) "xpath", sizeof("xpath")) == 0);
}

bool
Block::paramNode(const xmlNodePtr node) const {
    return (xmlStrncasecmp(node->name, (const xmlChar*) "param", sizeof("param")) == 0);
}

void
Block::parseXPathNode(const xmlNodePtr node) {
    const char *expr = XmlUtils::attrValue(node, "expr");
    const char *result = XmlUtils::attrValue(node, "result");
    if (expr && *expr && result && *result) {
        const char *delim = XmlUtils::attrValue(node, "delim");
        if (!delim || !*delim) {
            delim = " ";
        }
        data_->xpath_.push_back(XPathExpr(expr, result, delim));
        xmlNs* ns = node->nsDef;
        while (ns) {
            data_->xpath_.back().addNamespace((const char*)ns->prefix, (const char*)ns->href);
            ns = ns->next;
        }
    }
}

void
Block::parseParamNode(const xmlNodePtr node, ParamFactory *pf) {
    std::auto_ptr<Param> p = pf->param(this, node);
    processParam(p);
}

void
Block::processParam(std::auto_ptr<Param> p) {
    data_->params_.push_back(p.get());
	p.release();
}

Logger*
Block::log() const {
    return data_->extension_->getLogger();
}

const std::vector<Block::XPathExpr>&
Block::xpath() const {
    return data_->xpath_;
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

} // namespace xscript
