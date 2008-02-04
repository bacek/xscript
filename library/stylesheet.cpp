#include "settings.h"

#include <sstream>
#include <iterator>
#include <algorithm>
#include <stdexcept>
#include <set>

#include <boost/checked_delete.hpp>
#include <boost/current_function.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <libxslt/variables.h>
#include <libxslt/transform.h>
#include <libxslt/extensions.h>
#include <libxslt/attributes.h>
#include <libxml/uri.h>

#include "xscript/util.h"
#include "xscript/block.h"
#include "xscript/param.h"
#include "xscript/logger.h"
#include "xscript/object.h"
#include "xscript/context.h"
#include "xscript/stylesheet.h"
#include "xscript/xslt_extension.h"
#include "xscript/stylesheet_cache.h"
#include "xscript/stylesheet_factory.h"
#include "xscript/extension.h"
#include "xscript/vhost_data.h"
#include "xscript/checking_policy.h"

#include "details/extension_list.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

struct ContextData
{
	ContextData();
	Context *ctx;
	Stylesheet *stylesheet;
};

ContextData::ContextData() : ctx(NULL), stylesheet(NULL)
{
}


class XsltInitalizer
{
public:
	XsltInitalizer();

	static int XSCRIPT_NAMESPACE_SIZE;

private:
	static void* ExtraDataInit(xsltTransformContextPtr, const xmlChar*);
	static void ExtraDataShutdown(xsltTransformContextPtr, const xmlChar*, void *data);
};

static XsltInitalizer xsltInitalizer;

Stylesheet::Stylesheet(const std::string &name) :
	modified_(std::numeric_limits<time_t>::min()),
	name_(name), stylesheet_(NULL), blocks_()
{
}

Stylesheet::~Stylesheet() {
	for (std::map<xmlNodePtr, Block*>::iterator i = blocks_.begin(), end = blocks_.end(); i != end; ++i) {
		delete i->second;
	}
}

time_t
Stylesheet::modified() const {
	return modified_;
}

const std::string&
Stylesheet::name() const { 
	return name_;
}

XmlDocHelper
Stylesheet::apply(Object *obj, Context *ctx, const XmlDocHelper &doc) {

	log()->entering(BOOST_CURRENT_FUNCTION);
	
	XsltTransformContextHelper tctx(xsltNewTransformContext(stylesheet_.get(), doc.get()));
	XmlUtils::throwUnless(NULL != tctx.get());

	log()->debug("%s: transform context created\n", name().c_str());
	
	attachContextData(tctx.get(), ctx, this);

	const std::vector<Param*> &p = obj->xsltParams();
	if (!p.empty()) {
		tctx->globalVars = xmlHashCreate(p.size());
		log()->debug("param list contains %llu elements\n", static_cast<unsigned long long>(p.size()));

		typedef std::set<std::string> ParamSetType;
		ParamSetType unique_params;
		for (std::vector<Param*>::const_iterator it = p.begin(), end = p.end(); it != end; ++it) {
			const Param *param = *it;
			const std::string &id = param->id();
			std::pair<ParamSetType::iterator, bool> result = unique_params.insert(id);
			if (result.second == false) {
				CheckingPolicy::instance()->processError(std::string("duplicated xslt-param: ") + id);
			}
			else {
				std::string value = param->asString(ctx);
				if (value.empty()) {
					log()->debug("skip empty xslt-param %s\n", id.c_str());
				}
				else {
					log()->debug("add xslt-param %s: %s\n", id.c_str(), value.c_str());
					xsltQuoteOneUserParam(tctx.get(), (const xmlChar*) id.c_str(), 
						(const xmlChar*) value.c_str());
				}
			}
		}
	}
	XmlDocHelper newdoc(xsltApplyStylesheetUser(stylesheet_.get(), doc.get(), NULL, NULL, NULL, tctx.get()));
	log()->debug("%s: %s, checking result document\n", BOOST_CURRENT_FUNCTION, name_.c_str());
	XmlUtils::throwUnless(NULL != newdoc.get());
	return newdoc;
}

void
Stylesheet::parse() {
	
	namespace fs = boost::filesystem;
	
	fs::path path(name_);
	if (!fs::exists(path) || fs::is_directory(path)) {
		std::stringstream stream;
		stream << "can not open " << path.native_file_string();
		throw std::runtime_error(stream.str());
	}
	
	XmlCharHelper canonic_path(xmlCanonicPath((const xmlChar *) path.native_file_string().c_str()));
	XmlDocHelper doc(xmlReadFile(
		(const char*) canonic_path.get(), NULL, XML_PARSE_DTDATTR | XML_PARSE_DTDLOAD | XML_PARSE_NOENT));

	XmlUtils::throwUnless(NULL != doc.get());
	log()->debug("%s stylesheet %s document parsed\n", BOOST_CURRENT_FUNCTION,  name_.c_str());
	
	std::string type = detectContentType(doc);
	if (!type.empty()) {
		content_type_ = type;
	}

	stylesheet_ = XsltStylesheetHelper(xsltParseStylesheetDoc(doc.get()));
	XmlUtils::throwUnless(NULL != stylesheet_.get());
	
	parseNode(xmlDocGetRootElement(doc.release()));
	
	detectOutputMethod(stylesheet_);
	detectOutputEncoding(stylesheet_);

	stylesheet_->omitXmlDeclaration = 1;
	modified_ = fs::last_write_time(path);
}

void
Stylesheet::parseNode(xmlNodePtr node) {

	ExtensionList* elist = ExtensionList::instance();
	for ( ; node ; node = node->next) {
		if (XML_ELEMENT_NODE == node->type) {
			Extension *ext = elist->extension(node);
			if (NULL != ext) {
				
				log()->debug("%s, creating block %s\n", name().c_str(), ext->name());
				std::auto_ptr<Block> b = ext->createBlock(this, node);

				assert(b.get());
				b->parse();

				blocks_[node] = b.get();
				b.release();
			}
			else {
				parseNode(node->children);
			}
		}
	}
}


void
Stylesheet::detectOutputMethod(const XsltStylesheetHelper &sh) {
	
	if (NULL == sh->method || !xmlStrlen(sh->method)) {
		
		setupContentType("text/html");
		output_method_.assign("html");
		
		sh->methodURI = NULL;
		sh->method = xmlStrdup((const xmlChar*) "html");
	}
	else if (strncmp("xhtml", (const char*) sh->method, sizeof("xhtml")) == 0 && strncasecmp(XmlUtils::XSCRIPT_NAMESPACE, 
		(const char*) sh->methodURI, XsltInitalizer::XSCRIPT_NAMESPACE_SIZE) == 0)
	{
		setupContentType("text/html");
		output_method_.assign("xhtml");
	
		xmlFree(sh->method);
		xmlFree(sh->methodURI);
		
		sh->methodURI = NULL;
		sh->method = xmlStrdup((const xmlChar*) "xml");
	}
	else if (strncmp("wml", (const char*) sh->method, sizeof("wml")) == 0 && strncasecmp(XmlUtils::XSCRIPT_NAMESPACE, 
		(const char*) sh->methodURI, XsltInitalizer::XSCRIPT_NAMESPACE_SIZE) == 0)
	{
		setupContentType("text/vnd.wap.wml");
		output_method_.assign("wml");
		
		xmlFree(sh->method);
		xmlFree(sh->methodURI);
		
		sh->methodURI = NULL;
		sh->method = xmlStrdup((const xmlChar*) "xml");
	}

	else if (strncmp("xml", (const char*) sh->method, sizeof("xml")) == 0) { 
		setupContentType("text/xml");
	}
	else if (strncmp("html", (const char*) sh->method, sizeof("html")) == 0) {
		setupContentType("text/html");
	}
	else if (strncmp("text", (const char*) sh->method, sizeof("text")) == 0) {
		setupContentType("text/plain");
	}
	if (output_method_.empty() && NULL != sh->method) {
		output_method_.assign((const char*) sh->method);
	}
}

void
Stylesheet::detectOutputEncoding(const XsltStylesheetHelper &sh) {
	if (NULL == stylesheet_->encoding) {
		output_encoding_.assign(VirtualHostData::instance()->getOutputEncoding(NULL));
		stylesheet_->encoding = xmlStrdup((const xmlChar*)(output_encoding_.c_str()));
	}
	else {
		output_encoding_.assign((const char*) stylesheet_->encoding);
	}
}

void
Stylesheet::setupContentType(const char *type) {
	if (content_type_.empty()) {
		content_type_.assign(type);
	}
}

std::string
Stylesheet::detectContentType(const XmlDocHelper &doc) const {
	
	std::string res;
	XmlXPathContextHelper ctx(xmlXPathNewContext(doc.get()));
	XmlUtils::throwUnless(NULL != ctx.get());
	            
	XmlXPathObjectHelper object(xmlXPathEvalExpression((const xmlChar*) "//*[local-name()='output']/@content-type", ctx.get()));
	XmlUtils::throwUnless(NULL != object.get());

	if (NULL != object->nodesetval && 0 != object->nodesetval->nodeNr) {
		const char *val = XmlUtils::value(object->nodesetval->nodeTab[0]);
		if (NULL != val) {
			res.assign(val);
		}
	}
	return res;
}

void
Stylesheet::attachContextData(xsltTransformContextPtr tctx, Context *ctx, Stylesheet *stylesheet) {
	ContextData* data = static_cast<ContextData*>(xsltGetExtData(tctx, (const xmlChar*) XmlUtils::XSCRIPT_NAMESPACE));
	XmlUtils::throwUnless(NULL != data);
	data->ctx = ctx;
	data->stylesheet = stylesheet;
}

Context*
Stylesheet::getContext(xsltTransformContextPtr tctx) {
	ContextData* data = static_cast<ContextData*>(xsltGetExtData(tctx, (const xmlChar*) XmlUtils::XSCRIPT_NAMESPACE));
	XmlUtils::throwUnless(NULL != data);
	return data->ctx;
}

Stylesheet*
Stylesheet::getStylesheet(xsltTransformContextPtr tctx) {
	ContextData* data = static_cast<ContextData*>(xsltGetExtData(tctx, (const xmlChar*) XmlUtils::XSCRIPT_NAMESPACE));
	XmlUtils::throwUnless(NULL != data);
	return data->stylesheet;
}

boost::shared_ptr<Stylesheet>
Stylesheet::create(const std::string &name) {
	
	log()->debug("%s creating stylesheet %s\n", BOOST_CURRENT_FUNCTION, name.c_str());
	
	StylesheetCache *cache = StylesheetCache::instance();
	boost::shared_ptr<Stylesheet> stylesheet = cache->fetch(name);
	if (NULL != stylesheet.get()) {
		return stylesheet;
	}
	
	stylesheet = StylesheetFactory::instance()->create(name);
	stylesheet->parse();
	
	cache->store(name, stylesheet);
	return stylesheet;
	
}

Block*
Stylesheet::block(xmlNodePtr node) {
	std::map<xmlNodePtr, Block*>::iterator i = blocks_.find(node);
	if (i != blocks_.end()) {
		return i->second;
	}
	return NULL;
}

int XsltInitalizer::XSCRIPT_NAMESPACE_SIZE = 0;

XsltInitalizer::XsltInitalizer()  {
	xsltRegisterExtModule((const xmlChar*) XmlUtils::XSCRIPT_NAMESPACE, ExtraDataInit, ExtraDataShutdown);
	XSCRIPT_NAMESPACE_SIZE = strlen(XmlUtils::XSCRIPT_NAMESPACE) + 1;
}

void*
XsltInitalizer::ExtraDataInit(xsltTransformContextPtr, const xmlChar*) {
	
	void* data = new ContextData();
	log()->debug("%s, data is: %p\n", BOOST_CURRENT_FUNCTION, data);
	return data;
}

void
XsltInitalizer::ExtraDataShutdown(xsltTransformContextPtr, const xmlChar*, void* data) {
	
	log()->debug("%s, data is: %p\n", BOOST_CURRENT_FUNCTION, data);
	delete static_cast<ContextData*>(data);
}

} // namespace xscript
