#include "settings.h"

#include <cerrno>
#include <cctype>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <algorithm>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/checked_delete.hpp>
#include <boost/current_function.hpp>
#include <boost/tokenizer.hpp>

#include <boost/thread/xtime.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <libxml/xinclude.h>
#include <libxml/uri.h>

#include "xscript/util.h"
#include "xscript/block.h"
#include "xscript/logger.h"
#include "xscript/script.h"
#include "xscript/context.h"
#include "xscript/response.h"
#include "xscript/extension.h"
#include "xscript/stylesheet.h"
#include "xscript/script_cache.h"
#include "xscript/threaded_block.h"
#include "xscript/script_factory.h"

#include "details/param_factory.h"
#include "details/extension_list.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

Script::Script(const std::string &name) :
	modified_(std::numeric_limits<time_t>::min()), doc_(NULL), 
	name_(name), flags_(FLAG_FORCE_STYLESHEET), expire_time_delta_(300),
	stylesheet_node_(NULL)
{
}

Script::~Script() {
	std::for_each(blocks_.begin(), blocks_.end(), boost::checked_deleter<Block>());
}

const Block*
Script::block(const std::string &id, bool throw_error) const {
	try {
		for (std::vector<Block*>::const_iterator i = blocks_.begin(), end = blocks_.end(); i != end; ++i) {
			if ((*i)->id() == id) {
				return (*i);
			}
		}
		if (throw_error) {
			std::stringstream stream;
			stream << "requested block with nonexistent id: " << id;
			throw std::invalid_argument(stream.str());
		}
		return NULL;
	}
	catch (const std::exception &e) {
		log()->error("%s, %s, caught exception: %s", BOOST_CURRENT_FUNCTION, name().c_str(), e.what());
		throw;
	}
}

const std::string&
Script::header(const std::string &name) const {
	try {
		std::map<std::string, std::string>::const_iterator i = headers_.find(name);
		if (headers_.end() == i) {
			std::stringstream stream;
			stream << "requested nonexistent header: " << name;
			throw std::invalid_argument(stream.str());
		}
		return i->second;
	}
	catch (const std::exception &e) {
		log()->error("%s, caught exception: %s", BOOST_CURRENT_FUNCTION, e.what());
		throw;
	}
}

void
Script::parse() {
	
	namespace fs = boost::filesystem;
	
	fs::path path(name());
	if (!fs::exists(path) || fs::is_directory(path)) {
		std::stringstream stream;
		stream << "can not open " << path.native_file_string();
		throw std::runtime_error(stream.str());
	}

	XmlCharHelper canonic_path(xmlCanonicPath((const xmlChar *) path.native_file_string().c_str()));
	doc_ = XmlDocHelper(xmlReadFile(
		(const char*) canonic_path.get(), NULL, XML_PARSE_DTDATTR | XML_PARSE_DTDLOAD | XML_PARSE_NOENT));

	XmlUtils::throwUnless(NULL != doc_.get());
	if (NULL == doc_->children) {
		throw std::runtime_error("got empty xml doc");
	}
	XmlUtils::throwUnless(xmlXIncludeProcessFlags(doc_.get(), XML_PARSE_NOENT) >= 0);

	std::vector<xmlNodePtr> xscript_nodes;
	parseNode(doc_->children, xscript_nodes);
	parseXScriptNodes(xscript_nodes);
	parseBlocks();
	buildXScriptNodeSet(xscript_nodes);
	postParse();

	modified_ = fs::last_write_time(path);
}

time_t
Script::modified() const {
	return modified_;
}

const std::string&
Script::name() const {
	return name_;
}

std::string
Script::fullName(const std::string &name) const {
	return Xml::fullName(name);
}

void
Script::removeUnusedNodes(const XmlDocHelper &doc) {
	removeUnusedNode(doc, stylesheet_node_);
}
	
XmlDocHelper
Script::invoke(boost::shared_ptr<Context> ctx) {

	log()->info("%s, invoking %s", BOOST_CURRENT_FUNCTION, name().c_str());
	try {
		unsigned int count = 0;
		int to = countTimeout();
		ctx->expect(blocks_.size());
		for (std::vector<Block*>::iterator i = blocks_.begin(), end = blocks_.end(); i != end; ++i, ++count) {
			(*i)->invokeCheckThreaded(ctx, count);
		}
		ctx->wait(to);
		log()->debug("%s, finished to wait", BOOST_CURRENT_FUNCTION);
		addHeaders(ctx.get());
		return fetchResults(ctx.get());
	}
	catch (const std::exception &e) {
		log()->crit("%s, %s: caught exception: %s", BOOST_CURRENT_FUNCTION, name().c_str(), e.what());
		throw;
	}
}

void
Script::applyStylesheet(Context *ctx, XmlDocHelper &doc) {
	boost::shared_ptr<Stylesheet> stylesheet(static_cast<Stylesheet*>(NULL));
	if (!ctx->xsltName().empty()) {
		stylesheet = Stylesheet::create(ctx->xsltName());
	}
	else if (!xsltName().empty()) {
		stylesheet = Stylesheet::create(xsltName());
	}
	if (NULL != stylesheet.get()) {
		ctx->createDocumentWriter(stylesheet);
		Object::applyStylesheet(stylesheet, ctx, doc, false);
	}
}

boost::shared_ptr<Script>
Script::create(const std::string &name) {
	
	ScriptCache *cache = ScriptCache::instance();
	boost::shared_ptr<Script> script = cache->fetch(name);
	if (NULL != script.get()) {
		return script;
	}
	script = ScriptFactory::instance()->create(name);
	script->parse();

	cache->store(name, script);
	return script;
}

bool
Script::allowMethod(const std::string& value) const {
	if (!allow_methods_.empty() &&
		std::find(allow_methods_.begin(), allow_methods_.end(), value) == allow_methods_.end()) {
		return false;
	}

	return true;
}

void
Script::allowMethods(const char *value) {

	allow_methods_.clear();

	typedef boost::char_separator<char> Separator;
	typedef boost::tokenizer<Separator> Tokenizer;

	std::string method_list(value);
	Tokenizer tok(method_list, Separator(" "));
	for(Tokenizer::iterator it = tok.begin(), it_end = tok.end(); it != it_end; ++it) {
		allow_methods_.push_back(*it);
		std::vector<std::string>::reverse_iterator method = allow_methods_.rbegin();
		std::transform(method->begin(), method->end(), method->begin(), 
			boost::bind(&toupper, _1));
	}
}

void
Script::parseNode(xmlNodePtr node, std::vector<xmlNodePtr>& xscript_nodes) {
	ExtensionList* elist = ExtensionList::instance();
	for ( ; node ; node = node->next) {
		if (XML_PI_NODE == node->type) {
			if (xmlStrncasecmp(node->name, (const xmlChar*) "xml-stylesheet", sizeof("xml-stylesheet")) == 0) {
				if (NULL == stylesheet_node_) {
					log()->debug("%s, parse stylesheet", name().c_str());
					parseStylesheetNode(node);
				}
				else {
					log()->debug("%s, skip stylesheet", name().c_str());
				}
			}
			continue;
		}
		if (XML_ELEMENT_NODE == node->type) {
			if (xmlStrncasecmp(node->name, (const xmlChar*) "xscript", sizeof("xscript")) == 0) {
				xscript_nodes.push_back(node);
				continue;
			}
			Extension *ext = elist->extension(node);
			if (NULL != ext) {
				log()->debug("%s, creating block %s", name().c_str(), ext->name());
				std::auto_ptr<Block> b = ext->createBlock(this, node);

				assert(b.get());

				blocks_.push_back(b.get());
				b.release();
				continue;
			}
		}
		if (node->children) {
			parseNode(node->children, xscript_nodes);
		}
	}
}

void
Script::parseXScriptNode(const xmlNodePtr node) {

	log()->debug("parsing xscript node");

	XmlUtils::visitAttributes(node->properties, 
		boost::bind(&Script::property, this, _1, _2));

	xmlNodePtr child = node->children;
	ParamFactory *pf = ParamFactory::instance();

	for ( ; child; child = child->next) {
		if (child->name && xmlStrncasecmp(child->name, 
			(const xmlChar*) "add-headers", sizeof("add-headers")) == 0) {
			parseHeadersNode(child->children);
			continue;
		}
		else if (xsltParamNode(child)) {
			log()->debug("parsing xslt-param node from script");
			parseXsltParamNode(child, pf);
			continue;
		}
		else if (XML_ELEMENT_NODE == child->type) {
			const char *name = (const char *) child->name, *value = XmlUtils::value(child);
			if (name && value) {
				property(name, value);
			}
			continue;
		}
	}
}

void
Script::parseXScriptNodes(std::vector<xmlNodePtr>& xscript_nodes) {

	log()->debug("parsing xscript nodes");

	for (std::vector<xmlNodePtr>::reverse_iterator i = xscript_nodes.rbegin(), end = xscript_nodes.rend(); i != end; ++i) {
		parseXScriptNode(*i);
	}
}

void
Script::parseBlocks() {

	log()->debug("parsing blocks");

	bool is_threaded = threaded();
	for (std::vector<Block*>::iterator i = blocks_.begin(), end = blocks_.end(); i != end; ++i) {
		Block *block = *i;
		assert(block);
		block->threaded(is_threaded);
		block->parse();
	}
}

void
Script::buildXScriptNodeSet(std::vector<xmlNodePtr>& xscript_nodes) {

	log()->debug("build xscript node set");

	for (std::vector<xmlNodePtr>::iterator i = xscript_nodes.begin(), end = xscript_nodes.end(); i != end; ++i) {
		xscript_node_set_.insert(*i);
	}
	xscript_nodes.clear();
}

void
Script::parseHeadersNode(xmlNodePtr node) {
	while (node) {
		if (node->name && xmlStrncasecmp(node->name, 
			(const xmlChar*) "header", sizeof("header")) == 0) {
			const char *name = XmlUtils::attrValue(node, "name"), *value = XmlUtils::attrValue(node, "value");
			if (name && value) {
				headers_.insert(std::make_pair<std::string, std::string>(name, value));
			}
		}
		node = node->next;
	}
}

void
Script::parseStylesheetNode(const xmlNodePtr node) {
	if (node->content) {
		const xmlChar* href = xmlStrstr(node->content, (const xmlChar*) "href=");
		if (NULL != href) {
			href += sizeof("href=") - 1;
			const xmlChar* begin = xmlStrchr(href, '"'); 
			if (NULL != begin) {
				begin += 1;
				const xmlChar* end = xmlStrchr(begin, '"');
				if (NULL != end) {
					xsltName(std::string((const char*) begin, (const char*) end));
					stylesheet_node_ = node;
					return;
				}
			}
		}
	}
	throw std::runtime_error("can not parse stylesheet node");
}

int
Script::countTimeout() const {
	int timeout = 0;
	for (std::vector<Block*>::const_iterator i = blocks_.begin(), end = blocks_.end(); i != end; ++i) {
		const ThreadedBlock *tb = dynamic_cast<const ThreadedBlock*>(*i);
		log()->debug("%s, is block threaded: %d", BOOST_CURRENT_FUNCTION, static_cast<int>(NULL != tb));
		if (NULL != tb && tb->threaded()) {
			log()->debug("%s, got threaded block timeout: %d", BOOST_CURRENT_FUNCTION, tb->timeout());
			timeout = std::max(timeout, tb->timeout());
		}
	}
	return timeout;
}

void
Script::addHeaders(Context *ctx) const {
	Response *response = ctx->response();
	if (NULL == response) { // request can be null only when running tests
		return;
	}
	response->setHeader("Expires", HttpDateUtils::format(time(NULL) + expire_time_delta_));
	for (std::map<std::string, std::string>::const_iterator i = headers_.begin(), end = headers_.end(); i != end; ++i) {
		response->setHeader(i->first, i->second);
	}
	
}

XmlDocHelper
Script::fetchResults(Context *ctx) const {
	
	XmlDocHelper newdoc(xmlCopyDoc(doc_.get(), 1));
	XmlUtils::throwUnless(NULL != newdoc.get());
	
	unsigned int count = 0;
		
	xmlNodePtr node = xmlDocGetRootElement(doc_.get());
	assert(node);
			
	xmlNodePtr newnode = xmlDocGetRootElement(newdoc.get());
	assert(newnode);
			
	fetchRecursive(ctx, node, newnode, count);
	return newdoc;
}

void
Script::fetchRecursive(Context *ctx, xmlNodePtr node, xmlNodePtr newnode, unsigned int &count) const {

	while (node && count != blocks_.size()) {

		if (newnode == NULL) {
			throw std::runtime_error(std::string("internal error in node ") + (char*)node->name);
		}

		xmlNodePtr next = newnode->next;
		log()->debug("%s, blocks found: %d, %d", BOOST_CURRENT_FUNCTION, blocks_.size(), count);
		if (blocks_[count]->node() == node) {

			xmlDocPtr doc = ctx->result(count);
			assert(doc);
			
			xmlNodePtr result = xmlDocGetRootElement(doc);
			if (result) {
				// xmlUnlinkNode(result);
				// xmlReplaceNode(newnode, result);
				xmlReplaceNode(newnode, xmlCopyNode(result, 1));
			}
			else {
				xmlUnlinkNode(newnode);
			}
			xmlFreeNode(newnode);
			count++;
		}
		else if (xscript_node_set_.find(node) != xscript_node_set_.end() ) {
			replaceXScriptNode(newnode, ctx);
		}
		else if (node->children) {
			fetchRecursive(ctx, node->children, newnode->children, count);
		}

		node = node->next;
		newnode = next;
	}
}

void
Script::postParse() {
}

void
Script::property(const char *prop, const char *value) {
	
	log()->debug("%s, setting property: %s=%s", name().c_str(), prop, value);
	
	if (strncasecmp(prop, "all-threaded", sizeof("all-threaded")) == 0) {
		threaded(strncasecmp(value, "yes", sizeof("yes")) == 0);
	}
	else if (strncasecmp(prop, "need-auth", sizeof("need-auth")) == 0) {
		needAuth(strncasecmp(value, "yes", sizeof("yes")) == 0);
	}
	else if (strncasecmp(prop, "force-auth", sizeof("force-auth")) == 0) {
		forceAuth(strncasecmp(value, "yes", sizeof("yes")) == 0);
	}
	else if (strncasecmp(prop, "allow-methods", sizeof("allow-methods")) == 0) {
		allowMethods(value);
	}
	else if (strncasecmp(prop, "xslt-dont-apply", sizeof("xslt-dont-apply")) == 0) {
		forceStylesheet(strncasecmp(value, "yes", sizeof("yes")) != 0);
	}
	else if (strncasecmp(prop, "http-expire-time-delta", sizeof("http-expire-time-delta")) == 0) {
		expireTimeDelta(boost::lexical_cast<unsigned int>(value));
	}
	else {
		throw std::runtime_error(std::string("invalid script property: ").append(prop));
	}
}

void
Script::replaceXScriptNode(xmlNodePtr node, Context *ctx) const {
	(void)ctx;
	xmlUnlinkNode(node);
	xmlFreeNode(node);
}

void
Script::removeUnusedNode(const XmlDocHelper &doc, xmlNodePtr orig) {
	if (NULL == orig) {
		return;
	}
	xmlNodePtr node = doc_->children, newnode = doc->children;
	while (NULL != node) {
		if (removeUnusedNode(node, newnode, orig)) {
			return;	
		}
		else {
			newnode = newnode->next;
			node = node->next;
		}
	}
}

bool
Script::removeUnusedNode(xmlNodePtr node, xmlNodePtr newnode, xmlNodePtr orig) {
	if (NULL == node) {
		return false;
	}
	else if (orig == node) {
		xmlUnlinkNode(newnode);
		xmlFreeNode(newnode);
		return true;
	}
	else {
		return removeUnusedNode(node->children, newnode->children, orig);
	}
}

} // namespace xscript
