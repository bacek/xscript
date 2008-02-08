#include "settings.h"

#include <algorithm>
#include <stdexcept>
#include <boost/checked_delete.hpp>
#include <boost/current_function.hpp>

#include "xscript/util.h"
#include "xscript/param.h"
#include "xscript/logger.h"
#include "xscript/object.h"
#include "xscript/stylesheet.h"
#include "details/param_factory.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

Object::Object() 
{
}

Object::~Object() {
	std::for_each(params_.begin(), params_.end(), boost::checked_deleter<Param>());
}

void
Object::postParse() {
}

void
Object::xsltName(const std::string &value) {
	if (value.empty()) {
		xslt_name_.erase();
	}
	else if (value[0] == '/') {
		xslt_name_ = value;
	}
	else {
		xslt_name_ = fullName(value);
	}
}

void
Object::checkParam(const Param *param) const {
	if (param->id().empty()) {
		throw std::runtime_error("xsl param without id");
	}
}

bool
Object::xsltParamNode(const xmlNodePtr node) const {
	return node->name && xmlStrncasecmp(node->name, (const xmlChar*) "xslt-param", sizeof("xslt-param")) == 0;
}

void
Object::parseXsltParamNode(const xmlNodePtr node, ParamFactory *pf) {
	std::auto_ptr<Param> p = pf->param(this, node);
	log()->debug("%s, creating param %s", BOOST_CURRENT_FUNCTION, p->id().c_str());
	checkParam(p.get());
	params_.push_back(p.get());
	p.release();
}

void
Object::applyStylesheet(boost::shared_ptr<Stylesheet> sh, Context *ctx, XmlDocHelper &doc, bool need_copy) {
	
	assert(NULL != doc.get());
	try {
		if (need_copy) {
			XmlDocHelper newdoc = sh->apply(this, ctx, doc);
			doc = XmlDocHelper(xmlCopyDoc(newdoc.get(), 1));
		}
		else {
			doc = sh->apply(this, ctx, doc);
		}
		XmlUtils::throwUnless(NULL != doc.get());
	}
	catch (const std::exception &e) {
		log()->crit("caught exception while applying xslt [%s]: %s", sh->name().c_str(), e.what());
		throw;
	}
}

} // namespace xscript
