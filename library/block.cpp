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

#include "xscript/xml.h"
#include "xscript/util.h"
#include "xscript/state.h"
#include "xscript/param.h"
#include "xscript/block.h"
#include "xscript/logger.h"
#include "xscript/object.h"
#include "xscript/context.h"
#include "xscript/vhost_data.h"
#include "xscript/stylesheet.h"
#include "xscript/thread_pool.h"
#include "details/param_factory.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

Block::Block(Xml *owner, xmlNodePtr node) :
	owner_(owner), node_(node)
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
Block::threaded(bool value) {
}

void
Block::parse() {
	
	XmlUtils::visitAttributes(node_->properties, 
		boost::bind(&Block::property, this, _1, _2));
	
	ParamFactory *pf = ParamFactory::instance();
	xmlNodePtr node = node_->children;
	while (node) {
		if (!node->name) {
			continue;
		}
		else if (xpathNode(node)) {
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
		node = node->next;
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
	if (!checkGuard(ctx)) {
		return fakeResult();
	}
	try {
		boost::any a;
		XmlDocHelper doc(call(ctx, a));
		if (NULL == doc.get()) {
			log()->error("%s, got empty document", BOOST_CURRENT_FUNCTION);
			return errorResult("got empty document");
		}

		log()->debug("%s, got source document: %p", BOOST_CURRENT_FUNCTION, doc.get());
		applyStylesheet(ctx, doc);

		XmlUtils::throwUnless(NULL != doc.get());
		log()->debug("%s, got source document: %p", BOOST_CURRENT_FUNCTION, doc.get());

		evalXPath(ctx, doc);
		postCall(ctx, doc, a);
		return doc;
	}
	catch (const std::exception &e) {
		log()->error("%s, caught exception: %s", BOOST_CURRENT_FUNCTION, e.what());
		return errorResult(e.what());
	}
}

void
Block::invokeCheckThreaded(boost::shared_ptr<Context> ctx, unsigned int slot) {
	if (threaded()) {
		boost::function<void()> f = boost::bind(&Block::callInternalThreaded, this, ctx, slot);
		ThreadPool::instance()->invoke(f);
	}
	else {
		callInternal(ctx, slot);
	}
}

void
Block::applyStylesheet(Context *ctx, XmlDocHelper &doc) {
	if (!xsltName().empty()) {
		boost::shared_ptr<Stylesheet> sh = Stylesheet::create(xsltName());
		Object::applyStylesheet(sh, ctx, doc, true);
	}
}

XmlDocHelper
Block::errorResult(const char *error) const {
	
	XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
	XmlUtils::throwUnless(NULL != doc.get());

	xmlNodePtr node = xmlNewDocNode(doc.get(), NULL, (const xmlChar*) "error", (const xmlChar*) error);
	XmlUtils::throwUnless(NULL != node);
		
	xmlNewProp(node, (const xmlChar*) "method", (const xmlChar*) method_.c_str());
	xmlDocSetRootElement(doc.get(), node);
		
	return doc;
}

bool
Block::checkGuard(Context *ctx) const {
	boost::shared_ptr<State> state = ctx->state();
	if (!guard_.empty()) {
		return state->has(guard_) && state->asBool(guard_);
	}
	return true;
}

void
Block::evalXPath(Context *ctx, const XmlDocHelper &doc) const {
	
	XmlXPathContextHelper xctx(xmlXPathNewContext(doc.get()));
	XmlUtils::throwUnless(NULL != xctx.get());

	for (std::vector<XPathExpr>::const_iterator iter = xpath_.begin(), end = xpath_.end(); iter != end; ++iter) {
		
		XmlXPathObjectHelper object(xmlXPathEvalExpression((const xmlChar*) iter->get<0>().c_str(), xctx.get()));
		XmlUtils::throwUnless(NULL != object.get());
		
		if (NULL != object->nodesetval && 0 != object->nodesetval->nodeNr) {
			std::string val;
			for (int i = 0; i < object->nodesetval->nodeNr; ++i) {
				xmlNodePtr node = object->nodesetval->nodeTab[i];
				appendNodeValue(node, val);
				if (object->nodesetval->nodeNr - 1 != i) {
					val.append(iter->get<2>());
				}
			}
			if (!val.empty()) {
				boost::shared_ptr<State> state = ctx->state();
				state->checkName(iter->get<1>());
				state->setString(iter->get<1>(), val);
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
		id_.assign(value);
	}
	else if (strncasecmp(name, "guard", sizeof("guard")) == 0) {
		guard_.assign(value);
	}
	else if (strncasecmp(name, "method", sizeof("method")) == 0) {
		method_.assign(value);
	}
	else if (strncasecmp(name, "xslt", sizeof("xslt")) == 0) {
		xsltName(value);
	}
	else {
		std::stringstream stream;
		stream << "bad block attribute: " << name;
		throw std::invalid_argument(stream.str());
	}
}

void
Block::postCall(Context *ctx, const XmlDocHelper &doc, const boost::any &a) {
}

void
Block::callInternal(boost::shared_ptr<Context> ctx, unsigned int slot) {
	ctx->result(slot, invoke(ctx.get()).release());	
}

void
Block::callInternalThreaded(boost::shared_ptr<Context> ctx, unsigned int slot) {
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
		xpath_.push_back(boost::make_tuple<std::string, std::string, std::string>(expr, result, delim));
	}
}

void
Block::parseParamNode(const xmlNodePtr node, ParamFactory *pf) {
	std::auto_ptr<Param> p = pf->param(this, node);
	params_.push_back(p.get());
	p.release();
}

} // namespace xscript
