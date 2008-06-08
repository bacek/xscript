#include "settings.h"

#include <sstream>
#include <stdexcept>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/parser.h>
#include <libxml/xinclude.h>

#include "xscript/util.h"
#include "xscript/config.h"
#include "details/loader.h"
#include "xscript/logger_factory.h"
#include "xscript/logger.h"
#include "xscript/sanitizer.h"
#include "xscript/authorizer.h"
#include "xscript/thread_pool.h"
#include "xscript/vhost_data.h"
#include "xscript/checking_policy.h"
#include "xscript/doc_cache.h"
#include "xscript/script_cache.h"
#include "xscript/script_factory.h"
#include "xscript/stylesheet_cache.h"
#include "xscript/stylesheet_factory.h"
#include "xscript/control_extension.h"
#include "xscript/status_info.h"

#include "details/xml_config.h"
#include "details/extension_list.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

Config::Config() 
{
}

Config::~Config() {
}

void
Config::startup() {

	LoggerFactory::instance()->init(this);
	log()->debug("logger factory started");

	DocCache::instance()->init(this);
	log()->debug("doc cache started");
	
	XmlUtils::init(this);

	Loader::instance()->init(this);
	log()->debug("loader started");
	
	StatusInfo::instance()->init(this);
	log()->debug("status info started");

	Authorizer::instance()->init(this);
	log()->debug("authorizer started");

	Sanitizer::instance()->init(this);
	log()->debug("sanitizer started");

	ThreadPool::instance()->init(this);
	log()->debug("thread pool started");
	
	VirtualHostData::instance()->init(this);
	log()->debug("virtual host data started");

	CheckingPolicyStarter::instance()->init(this);
	log()->debug("checking policy started");

	ScriptCache::instance()->init(this);	
	log()->debug("script cache started");
	
	ScriptFactory::instance()->init(this);
	log()->debug("script factory started");
	
	StylesheetCache::instance()->init(this);	
	log()->debug("stylesheet cache started");
	
	StylesheetFactory::instance()->init(this);
	log()->debug("stylesheet factory started");
	
	ExtensionList::instance()->registerExtension(ExtensionHolder(new ControlExtension));
	
	ExtensionList::instance()->init(this);
	log()->debug("extension list started");
}

std::auto_ptr<Config>
Config::create(const char *file) {
	return std::auto_ptr<Config>(new XmlConfig(file));
}

std::auto_ptr<Config>
Config::create(int &argc, char *argv[], HelpFunc func) {
	for (int i = 1; i < argc; ++i) {
		if (strncmp(argv[i], "--config", sizeof("--config") - 1) == 0) {
			const char *pos = strchr(argv[i], '=');
			if (NULL != pos) {
				std::auto_ptr<Config> conf(new XmlConfig(pos + 1));
				std::swap(argv[i], argv[argc - 1]);
				--argc;
				return conf;
			}
		}
	}
	std::stringstream stream;
	if (NULL != func) {
		func(stream);
	}
	else {
		stream << "usage: xscript --config=<config file>";
	}
	throw std::logic_error(stream.str());
}

XmlConfig::XmlConfig(const char *file) :
	doc_(NULL), regex_("\\$\\{([A-Za-z][A-Za-z0-9\\-]*)\\}")
{
	namespace fs = boost::filesystem;
	boost::filesystem::path path(file, fs::no_check);
	if (!fs::exists(path)) {
		std::stringstream stream;
		stream << "can not read " << path.native_file_string();
		throw std::runtime_error(stream.str());
	}
		
	doc_ = XmlDocHelper(xmlParseFile(path.native_file_string().c_str()));
	XmlUtils::throwUnless(NULL != doc_.get());
	if (NULL == xmlDocGetRootElement(doc_.get())) {
		throw std::logic_error("got empty config");
	}
	XmlUtils::throwUnless(xmlXIncludeProcess(doc_.get()) >= 0);
	findVariables(doc_);
}

XmlConfig::~XmlConfig() {
}

std::string
XmlConfig::value(const std::string &key) const {

	std::string res;
	
	XmlXPathContextHelper xctx(xmlXPathNewContext(doc_.get()));
	XmlUtils::throwUnless(NULL != xctx.get());

	XmlXPathObjectHelper object(xmlXPathEvalExpression((const xmlChar*) key.c_str(), xctx.get()));
	XmlUtils::throwUnless(NULL != object.get());
		
	if (NULL != object->nodesetval && 0 != object->nodesetval->nodeNr) {
			
		xmlNodeSetPtr ns = object->nodesetval;
		XmlUtils::throwUnless(NULL != ns->nodeTab[0]);
		const char *val = XmlUtils::value(ns->nodeTab[0]);
		if (NULL != val) {
			res.assign(val);
		}
	}
	else {
		std::stringstream stream;
		stream << "nonexistent config param: " << key;
		throw std::runtime_error(stream.str());
	}
	resolveVariables(res);
	return res;
}

void
XmlConfig::subKeys(const std::string &key, std::vector<std::string> &v) const {
	
	XmlXPathContextHelper xctx(xmlXPathNewContext(doc_.get()));
	XmlUtils::throwUnless(NULL != xctx.get());

	XmlXPathObjectHelper object(xmlXPathEvalExpression((const xmlChar*) key.c_str(), xctx.get()));
	XmlUtils::throwUnless(NULL != object.get());
		
	if (NULL != object->nodesetval && 0 != object->nodesetval->nodeNr) {
			
		xmlNodeSetPtr ns = object->nodesetval;
		v.reserve(ns->nodeNr);
			
		for (int i = 0; i < ns->nodeNr; ++i) {
			XmlUtils::throwUnless(NULL != ns->nodeTab[i]);
			std::stringstream stream;
			stream << key << "[" << (i + 1) << "]";
			v.push_back(stream.str());
		}
	}
}

void
XmlConfig::findVariables(const XmlDocHelper &doc) {
	
	XmlXPathContextHelper xctx(xmlXPathNewContext(doc.get()));
	XmlUtils::throwUnless(NULL != xctx.get());
	
	XmlXPathObjectHelper object(xmlXPathEvalExpression((const xmlChar*) "/xscript/variables/variable", xctx.get()));
	XmlUtils::throwUnless(NULL != object.get());
	
	if (NULL != object->nodesetval && 0 != object->nodesetval->nodeNr) {
		xmlNodeSetPtr ns = object->nodesetval;
		for (int i = 0; i < ns->nodeNr; ++i) {
			
			xmlNodePtr node = ns->nodeTab[i];
			XmlUtils::throwUnless(NULL != node);
			
			const char *val = XmlUtils::value(node);
			const char *name = XmlUtils::attrValue(node, "name");
			if (NULL == val || NULL == name) {
				throw std::logic_error("bad variable definition");
			}
			vars_.insert(std::pair<std::string, std::string>(name, val));
		}
	}
}

void
XmlConfig::resolveVariables(std::string &val) const {
	boost::smatch res;
	while (boost::regex_search(val, res, regex_)) {
		if (2 == res.size()) {
			std::string key(res[1].first, res[1].second);
			val.replace(res.position(0), res.length(0), findVariable(key));
		}
	}
}

const std::string&
XmlConfig::findVariable(const std::string &key) const {
	VarMap::const_iterator i = vars_.find(key);
	if (vars_.end() != i) {
		return i->second;
	}
	else {
		throw std::runtime_error(std::string("nonexistent variable ").append(key));
	}
}

} // namespace xscript
