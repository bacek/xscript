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
#include "xscript/thread_pool.h"
#include "xscript/vhost_data.h"
#include "xscript/xml.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class XPathExpr {
public:
    XPathExpr(const char* expression, const char* result, const char* delimeter, const char* type);
    ~XPathExpr();

    typedef std::list<std::pair<std::string, std::string> > NamespaceListType;

    std::string expression(Context *ctx) const;
    const std::string& result() const;
    const std::string& delimeter() const;
    const NamespaceListType& namespaces() const;
    void addNamespace(const char* prefix, const char* uri);

private:
    std::string expression_;
    std::string result_;
    std::string delimeter_;
    NamespaceListType namespaces_;
    bool from_state_;
};

class Guard {
public:
    Guard(const char *expr, const char *type, const char *value, bool is_not);
    bool check(const Context *ctx);

private:
    std::string guard_;
    std::string value_;
    bool not_;
    GuardCheckerMethod method_;
    static const std::string STATE_ARG_PARAM_NAME;
};

const std::string Guard::STATE_ARG_PARAM_NAME = "StateArg";

struct Block::BlockData {
    BlockData(const Extension *ext, Xml *owner, xmlNodePtr node) :
        extension_(ext), owner_(owner), node_(node)
    {}
    
    ~BlockData() {
        std::for_each(params_.begin(), params_.end(), boost::checked_deleter<Param>());
    }
    
    const Extension *extension_;
    Xml *owner_;
    xmlNodePtr node_;
    std::vector<Param*> params_;
    std::vector<XPathExpr> xpath_;
    std::vector<Guard> guards_;
    std::string id_, method_;
    std::string xpointer_expr_;
    std::string base_;
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
Block::detectBase() {

    XmlCharHelper base(xmlNodeGetBase(data_->node_->doc, data_->node_));
    data_->base_ = (const char*)base.get();
    
    std::string::size_type pos = data_->base_.find_last_of('/');
    if (pos != std::string::npos) {
        data_->base_.erase(pos + 1);
    }
}

void
Block::parse() {
    try {       
        detectBase();
        
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
        postParse();
    }
    catch(const std::exception &e) {
        log()->error("%s, parse failed for '%s': %s", name(), owner()->name().c_str(), e.what());
        throw;
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

InvokeResult
Block::invoke(boost::shared_ptr<Context> ctx) {
    log()->debug("%s", BOOST_CURRENT_FUNCTION);

    if (ctx->stopped()) {
        log()->error("Context already stopped. Cannot invoke block. Owner: %s. Block: %s. Method: %s",
            owner()->name().c_str(), name(), data_->method_.c_str());
        return fakeResult();
    }

    if (!checkGuard(ctx.get())) {
        log()->info("Guard skipped block processing. Owner: %s. Block: %s. Method: %s",
            owner()->name().c_str(), name(), data_->method_.c_str());
        return fakeResult();
    }

    InvokeResult result;
    try {
        BlockTimerStarter starter(ctx.get(), this);
        result = invokeInternal(ctx);
        if (!result.error()) {
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
        result = fakeResult();
    }
    catch (const InvokeError &e) {
        result = errorResult(e.what(), e.info());
    }
    catch (const std::exception &e) {
        result = errorResult(e.what());
    }
    
    if (!result.success()) {
        ctx->rootContext()->setNoCache();
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

    return processResponse(ctx, doc, a);
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

InvokeResult
Block::processResponse(boost::shared_ptr<Context> ctx, XmlDocHelper doc, boost::any &a) {
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
    bool need_perblock = !ctx->noXsltPort();
    if (need_perblock) {
        applyStylesheet(ctx, doc);
    }

    InvokeResult::Type type =
        is_error_doc ? InvokeResult::NO_CACHE : InvokeResult::SUCCESS;
    
    InvokeResult result(doc, type);
    
    postCall(ctx.get(), result, a);

    if (need_perblock || xsltName().empty()) {
        evalXPath(ctx.get(), result.doc);
    }

    return result;
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

    return InvokeResult(doc, InvokeResult::ERROR);
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

InvokeResult
Block::fakeResult() const {
    return InvokeResult(fakeDoc(), InvokeResult::ERROR);
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

void
Block::evalXPath(Context *ctx, const XmlDocHelper &doc) const {

    XmlXPathContextHelper xctx(xmlXPathNewContext(doc.get()));
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
        
        const XPathExpr::NamespaceListType& ns_list = iter->namespaces();
        for (XPathExpr::NamespaceListType::const_iterator it_ns = ns_list.begin(); it_ns != ns_list.end(); ++it_ns) {
            xmlXPathRegisterNs(xctx.get(), (const xmlChar *)it_ns->first.c_str(), (const xmlChar *)it_ns->second.c_str());
        }

        XmlXPathObjectHelper object(xmlXPathEvalExpression((const xmlChar*)expr.c_str(), xctx.get()));
        try {
            XmlUtils::throwUnless(NULL != object.get());
        }
        catch(const std::exception &e) {
            throw InvokeError(e.what(), "xpath", expr);
        }

        //log()->debug("%s, xpath: %s type=%d bool=%d float=%f str='%s'",
        //    BOOST_CURRENT_FUNCTION, iter->expression().c_str(), object->type, object->boolval, object->floatval,
        //    NULL !=object->stringval ? (const char *)object->stringval: "");

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

    log()->debug("setting %s=%s", name, value);

    if (strncasecmp(name, "id", sizeof("id")) == 0) {
        if (!data_->id_.empty()) {
            throw std::runtime_error("duplicated id in block");
        }
        data_->id_.assign(value);
    }
    else if (strncasecmp(name, "guard", sizeof("guard")) == 0) {
        if (*value == '\0') {
            throw std::runtime_error("empty guard");
        }
        data_->guards_.push_back(Guard(value, NULL, NULL, false));
    }
    else if (strncasecmp(name, "guard-not", sizeof("guard-not")) == 0) {
        if (*value == '\0') {
            throw std::runtime_error("empty guard-not");
        }
        data_->guards_.push_back(Guard(value, NULL, NULL, true));
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
Block::postCall(Context *, const InvokeResult &, const boost::any &) {
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
        if (!delim || !*delim) {
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
Block::parseGuardNode(const xmlNodePtr node, bool is_not) {
    const char *guard = XmlUtils::value(node);
    const char *type = XmlUtils::attrValue(node, "type");
    const char *value = XmlUtils::attrValue(node, "value");
    data_->guards_.push_back(Guard(guard, type, value, is_not));
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
    not_(is_not),
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
}

bool
Guard::check(const Context *ctx) {
    return not_ ^ (*method_)(ctx, guard_, value_);
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

const XPathExpr::NamespaceListType&
XPathExpr::namespaces() const {
    return namespaces_;
}

void
XPathExpr::addNamespace(const char* prefix, const char* uri) {
    if (prefix && uri) {
        namespaces_.push_back(std::make_pair(std::string(prefix), std::string(uri)));
    }
}

} // namespace xscript
