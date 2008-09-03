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

Block::Block(const Extension *ext, Xml *owner, xmlNodePtr node) :
        extension_(ext), owner_(owner), node_(node), is_guard_not_(false), strip_root_element_(false)
{
    assert(node_);
    assert(owner_);
}

Block::~Block() {
    std::for_each(params_.begin(), params_.end(), boost::checked_deleter<Param>());
}

const Param*
Block::param(const std::string &id, bool throw_error) const {
    try {
        for (std::vector<Param*>::const_iterator i = params_.begin(), end = params_.end(); i != end; ++i) {
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
Block::stripRootElement(Context* ctx) const {
    if (strip_root_element_) {
        const Server* server = VirtualHostData::instance()->getServer();
        if (!xsltName().empty() && server && !server->needApplyPerblockStylesheet(ctx->request())) {
            return false;
        }
    }
    return strip_root_element_;
}

void
Block::parse() {

    XmlUtils::visitAttributes(node_->properties,
                              boost::bind(&Block::property, this, _1, _2));

    ParamFactory *pf = ParamFactory::instance();
    for (xmlNodePtr node = node_->children; NULL != node; node = node->next) {
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

std::string
Block::fullName(const std::string &name) const {
    return owner()->fullName(name);
}


XmlDocHelper
Block::invoke(Context *ctx) {
    log()->debug("%s", BOOST_CURRENT_FUNCTION);
    if (ctx->stopped()) {
        log()->error("Context already stopped. Cannot invoke block. Method: %s", method_.c_str());
        return fakeResult();
    }

    if (!checkGuard(ctx)) {
        return fakeResult();
    }

    return invokeInternal(ctx);
}

XmlDocHelper
Block::invokeInternal(Context *ctx) {

    log()->debug("%s", BOOST_CURRENT_FUNCTION);

    try {
        boost::any a;
        XmlDocHelper doc(call(ctx, a));

        if (NULL == doc.get()) {
            log()->error("%s, got empty document", BOOST_CURRENT_FUNCTION);
            return errorResult("got empty document");
        }

        return processResponse(ctx, doc, a);
    }
    catch (const XmlNodeRuntimeError &e) {
        log()->error("%s, caught exception: %s. Owner: %s", BOOST_CURRENT_FUNCTION, e.what(), owner()->name().c_str());
        return errorResult(e.what_node());
    }
    catch (const std::exception &e) {
        log()->error("%s, caught exception: %s. Owner: %s", BOOST_CURRENT_FUNCTION, e.what(), owner()->name().c_str());
        return errorResult(e.what());
    }
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
Block::processResponse(Context *ctx, XmlDocHelper doc, boost::any &a) {
    if (NULL == doc.get()) {
        throw std::runtime_error("Null response document");
    }

    if (NULL == xmlDocGetRootElement(doc.get())) {
        log()->error("%s, got document with no root", BOOST_CURRENT_FUNCTION);
        return errorResult("got document with no root");
    }

    if (!tagged() && ctx->stopped()) {
        throw std::runtime_error(
            std::string("Context already stopped. Cannot process response. Method: ") + method_);
    }

    log()->debug("%s, got source document: %p", BOOST_CURRENT_FUNCTION, doc.get());
    const Server* server = VirtualHostData::instance()->getServer();
    bool need_perblock = (!server || server->needApplyPerblockStylesheet(ctx->request()));
    if (need_perblock) {
        applyStylesheet(ctx, doc);
    }

    postCall(ctx, doc, a);

    if (need_perblock || xsltName().empty()) {
        evalXPath(ctx, doc);
    }

    return doc;
}

void
Block::applyStylesheet(Context *ctx, XmlDocHelper &doc) {
    if (!xsltName().empty()) {
        boost::shared_ptr<Stylesheet> sh = Stylesheet::create(xsltName());
        {
            PROFILER(log(), std::string("per-block-xslt: '") + xsltName() + "' block-id: '" + id() + "' method: '" + method() + "' owner: '" + owner()->name() + "'");
            Object::applyStylesheet(sh, ctx, doc, true);
        }

        XmlUtils::throwUnless(NULL != doc.get());
        log()->debug("%s, got source document: %p", BOOST_CURRENT_FUNCTION, doc.get());
    }
}

XmlDocHelper
Block::errorResult(const char *error, xmlNodePtr error_node) const {

    XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != doc.get());

    xmlNodePtr node = xmlNewDocNode(doc.get(), NULL, (const xmlChar*) "error", (const xmlChar*)error);
    XmlUtils::throwUnless(NULL != node);

    xmlNewProp(node, (const xmlChar*) "method", (const xmlChar*) method_.c_str());
    xmlDocSetRootElement(doc.get(), node);

    if (error_node != NULL) {
        xmlNodePtr result_node = error_node->children;
        while (result_node) {
            xmlAddChild(node, xmlCopyNode(result_node, 1));
            result_node = result_node->next;
        }
    }

    return doc;
}

XmlDocHelper
Block::errorResult(xmlNodePtr error_node) const {
    return errorResult(NULL, error_node);
}

XmlDocHelper
Block::errorResult(const char *error) const {
    return errorResult(error, NULL);
}

bool
Block::checkGuard(Context *ctx) const {
    if (!guard_.empty()) {
        return is_guard_not_ ^ ctx->state()->is(guard_);
    }
    return true;
}

void
Block::evalXPath(Context *ctx, const XmlDocHelper &doc) const {

    XmlXPathContextHelper xctx(xmlXPathNewContext(doc.get()));
    XmlUtils::throwUnless(NULL != xctx.get());

    for (std::vector<XPathExpr>::const_iterator iter = xpath_.begin(), end = xpath_.end(); iter != end; ++iter) {
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
        if (!id_.empty()) {
            throw std::runtime_error("duplicated id in block");
        }
        id_.assign(value);
    }
    else if (strncasecmp(name, "guard", sizeof("guard")) == 0) {
        if (!guard_.empty()) {
            throw std::runtime_error("duplicated guard in block");
        }
        if (*value == '\0') {
            throw std::runtime_error("empty guard");
        }
        guard_.assign(value);
    }
    else if (strncasecmp(name, "guard-not", sizeof("guard-not")) == 0) {
        if (!guard_.empty()) {
            throw std::runtime_error("duplicated guard in block");
        }
        if (*value == '\0') {
            throw std::runtime_error("empty guard-not");
        }
        guard_.assign(value);
        is_guard_not_ = true;
    }
    else if (strncasecmp(name, "method", sizeof("method")) == 0) {
        if (!method_.empty()) {
            throw std::runtime_error("duplicated method in block");
        }
        method_.assign(value);
    }
    else if (strncasecmp(name, "xslt", sizeof("xslt")) == 0) {
        xsltName(value);
    }
    else if (strncasecmp(name, "strip-root-element", sizeof("strip-root-element")) == 0) {
        stripRootElement((strncasecmp(value, "yes", sizeof("yes")) == 0));
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
Block::callInternal(boost::shared_ptr<Context> ctx, unsigned int slot) {
    ctx->result(slot, invoke(ctx.get()).release());
}

void
Block::callInternalThreaded(boost::shared_ptr<Context> ctx, unsigned int slot) {
    XmlUtils::registerReporters();
    VirtualHostData::instance()->set(ctx->request());
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
        xpath_.push_back(XPathExpr(expr, result, delim));
        xmlNs* ns = node->nsDef;
        while (ns) {
            xpath_.back().addNamespace((const char*)ns->prefix, (const char*)ns->href);
            ns = ns->next;
        }
    }
}

void
Block::parseParamNode(const xmlNodePtr node, ParamFactory *pf) {
    std::auto_ptr<Param> p = pf->param(this, node);
    params_.push_back(p.get());
    p.release();
}

} // namespace xscript
