#include "settings.h"

#include <sstream>
#include <stdexcept>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/parser.h>
#include <libxml/xinclude.h>

#include "xscript/algorithm.h"
#include "xscript/authorizer.h"
#include "xscript/config.h"
#include "xscript/control_extension.h"
#include "xscript/doc_cache_strategy.h"
#include "xscript/logger_factory.h"
#include "xscript/logger.h"
#include "xscript/policy.h"
#include "xscript/sanitizer.h"
#include "xscript/script_cache.h"
#include "xscript/script_factory.h"
#include "xscript/status_info.h"
#include "xscript/stylesheet_cache.h"
#include "xscript/stylesheet_factory.h"
#include "xscript/thread_pool.h"
#include "xscript/vhost_data.h"
#include "xscript/xml_helpers.h"
#include "xscript/xml_util.h"

#include "details/xml_config.h"

#include "internal/extension_list.h"
#include "internal/hash.h"
#include "internal/hashmap.h"
#include "internal/loader.h"

#ifndef HAVE_HASHMAP
#include <map>
#endif

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

#ifndef HAVE_HASHMAP
typedef std::map<std::string, std::string> VarMap;
#else
typedef details::hash_map<std::string, std::string, details::StringHash> VarMap;
#endif

static std::map<std::string, std::string>* cache_params = NULL;
static std::set<std::string>* forbidden_keys = NULL;
static bool stop_collect_cache = false;

Config::Config() {
    if (NULL == cache_params) {
        cache_params = new std::map<std::string, std::string>();
    }
    if (NULL == forbidden_keys) {
        forbidden_keys = new std::set<std::string>();
    }
}

Config::~Config() {
    if (NULL != cache_params) {
        delete cache_params;
        cache_params = NULL;
    }
    if (NULL != forbidden_keys) {
        delete forbidden_keys;
        forbidden_keys = NULL;
    }
}

void
Config::startup() {

    ConfigParams::init(this);
    
    LoggerFactory::instance()->init(this);
    log()->debug("logger factory started");

    XmlUtils::init(this);
    
    Loader::instance()->init(this);
    log()->debug("loader started");
    
    CacheStrategyCollector::instance()->init(this);
    log()->debug("doc and page cache started");
    
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
Config::create(int &argc, char *argv[], bool dont_check, HelpFunc func) {
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

    if (dont_check) {
        return std::auto_ptr<Config>();
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

void
Config::addForbiddenKey(const std::string &key) {
    if (stop_collect_cache) {
        return;
    }
    Range key_r = trim(createRange(key));
    if (key_r.empty()) {
        return;
    }
    
    static Range mask = createRange("/*");
    if (endsWith(key_r, mask)) {
        forbidden_keys->insert(std::string(key_r.begin(), key_r.end() - 1));
    }
    else {
        forbidden_keys->insert(std::string(key_r.begin(), key_r.end()));
    }
}

void
Config::addCacheParam(const std::string &name, const std::string &value) {
    if (stop_collect_cache) {
        return;
    }
    Range name_r = trim(createRange(name));
    static Range prefix = createRange("/xscript/");
    if (!startsWith(name_r, prefix)) {
        return;
    }
    
    for(std::set<std::string>::iterator it = forbidden_keys->begin();
        it != forbidden_keys->end();
        ++it) {
        if ('/' == *it->rbegin()) {
            if (0 == strncmp(name_r.begin(), it->c_str(), it->size())) {
                return;
            }
        }
        else if (name_r == trim(createRange(*it))) {
            return;
        }
    }
    
    std::string key(name_r.begin() + prefix.size(), name_r.end());
    (*cache_params)[key] = value;
}

bool
Config::getCacheParam(const std::string &name, std::string &value) {
    std::map<std::string, std::string>::iterator it = cache_params->find(name);
    if (cache_params->end() == it) {
        return false;
    }
    value = it->second;
    return true;
}

void
Config::stopCollectCache() {
    stop_collect_cache = true;
}

class XmlConfig::XmlConfigData {
public:
    XmlConfigData(const char *file);
    ~XmlConfigData();
    
    void findVariables(const XmlDocHelper &doc);
    void resolveVariables(std::string &val) const;
    const std::string& findVariable(const std::string &key) const;
    bool checkVariableName(const std::string &name) const;
    
    VarMap vars_;
    XmlDocHelper doc_;
};

XmlConfig::XmlConfigData::XmlConfigData(const char *file) : doc_(NULL) {
    namespace fs = boost::filesystem;
    fs::path path(file, fs::no_check);
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

XmlConfig::XmlConfigData::~XmlConfigData()
{}

void
XmlConfig::XmlConfigData::findVariables(const XmlDocHelper &doc) {

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
XmlConfig::XmlConfigData::resolveVariables(std::string &val) const {
    size_t pos_begin = std::string::npos;
    while(true) {
        pos_begin = val.rfind("${", pos_begin);
        if (pos_begin == std::string::npos) {
            return;
        }
        size_t pos_end = val.find('}', pos_begin);
        std::string name = val.substr(pos_begin + 2, pos_end - pos_begin - 2);
        if (checkVariableName(name)) {
            val.replace(pos_begin, pos_end - pos_begin + 1, findVariable(name));
        }
        else {
            throw std::runtime_error("incorrect variable name format: " + name);
        }
    }
}

const std::string&
XmlConfig::XmlConfigData::findVariable(const std::string &key) const {
    VarMap::const_iterator i = vars_.find(key);
    if (vars_.end() != i) {
        return i->second;
    }
    else {
        throw std::runtime_error(std::string("nonexistent variable ").append(key));
    }
}

bool
XmlConfig::XmlConfigData::checkVariableName(const std::string &name) const {

    if (name.empty()) {
        return false;
    }

    if (!isalpha(name[0])) {
        return false;
    }

    int size = name.size();
    for (int i = 1; i < size; ++i) {
        char character = name[i];
        if (!isalnum(character) && character != '-' && character != '_') {
            return false;
        }
    }

    return true;
}

XmlConfig::XmlConfig(const char *file) : data_(new XmlConfigData(file))
{}

XmlConfig::~XmlConfig() {
    delete data_;
}

std::string
XmlConfig::value(const std::string &key) const {

    std::string res;

    XmlXPathContextHelper xctx(xmlXPathNewContext(data_->doc_.get()));
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
    data_->resolveVariables(res);
    return res;
}

void
XmlConfig::subKeys(const std::string &key, std::vector<std::string> &v) const {

    XmlXPathContextHelper xctx(xmlXPathNewContext(data_->doc_.get()));
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

static int default_timeout = 5000;

void 
ConfigParams::init(const Config *config) {
    Config::addForbiddenKey("/xscript/default-block-timeout");
    default_timeout = config->as<int>("/xscript/default-block-timeout", 5000);
}

int
ConfigParams::defaultTimeout() {
    return default_timeout;
}

} // namespace xscript
