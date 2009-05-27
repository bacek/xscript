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

#include "xscript/block.h"
#include "xscript/context.h"
#include "xscript/doc_cache.h"
#include "xscript/extension.h"
#include "xscript/logger.h"
#include "xscript/operation_mode.h"
#include "xscript/policy.h"
#include "xscript/profiler.h"
#include "xscript/response.h"
#include "xscript/script.h"
#include "xscript/script_cache.h"
#include "xscript/script_factory.h"
#include "xscript/stylesheet.h"
#include "xscript/threaded_block.h"
#include "xscript/xml_util.h"

#include "internal/param_factory.h"
#include "internal/extension_list.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

static const time_t CACHE_TIME_UNDEFINED = std::numeric_limits<time_t>::max();
const std::string Script::GET_METHOD = "GET";

class Script::ScriptData {
public:
    ScriptData(const std::string &name);
    ~ScriptData();
    
    bool threaded() const;
    bool forceStylesheet() const;
    bool binaryPage() const;
    unsigned int expireTimeDelta() const;
    time_t cacheTime() const;
    boost::int32_t pageRandomMax() const;
    bool cacheTimeUndefined() const;
    bool allowMethod(const std::string& value) const;
    bool cacheAllQuery() const;
    bool cacheQueryParam(const std::string &value) const;
    const std::string& name() const;
    
    const std::string& header(const std::string &name) const;
    const std::map<std::string, std::string>& headers() const;
    const std::string& extensionProperty(const std::string &name) const;
    std::set<xmlNodePtr>& xscriptNodes() const;
    
    unsigned int blocksNumber() const;
    Block* block(unsigned int n) const;
    Block* block(const std::string &id, bool throw_error) const;
    std::vector<Block*>& blocks() const;
    std::set<std::string>& cacheCookies() const;
    
    void addBlock(Block *block);
    void addXscriptNode(xmlNodePtr node);
    void addHeader(const std::string &name, const std::string &value);
    void setHeaders(Response *response);
    void extensionProperty(const std::string &name, const std::string &value);

    void threaded(bool value);
    void forceStylesheet(bool value);
    void expireTimeDelta(unsigned int value);
    void cacheTime(time_t value);
    void pageRandomMax(boost::int32_t value);
    void binaryPage(bool value);
    void allowMethods(const char *value);
    void cacheQuery(const char *value);
    void cacheCookies(const char *value);
    void flag(unsigned int type, bool value);
    
    XmlDocHelper doc_;
    std::vector<Block*> blocks_;
    std::string name_;
    unsigned int flags_;
    unsigned int expire_time_delta_;
    time_t cache_time_;
    boost::int32_t page_random_max_;
    std::set<xmlNodePtr> xscript_node_set_;
    std::map<std::string, std::string> headers_;
    std::vector<std::string> allow_methods_;
    std::set<std::string> cache_query_;
    std::set<std::string> cache_cookies_;
    std::map<std::string, std::string, StringCILess> extension_properties_;
    
    static const unsigned int FLAG_THREADED = 1;
    static const unsigned int FLAG_FORCE_STYLESHEET = 1 << 1;
    static const unsigned int FLAG_BINARY_PAGE = 1 << 2;
};

Script::ScriptData::ScriptData(const std::string &name) :
    doc_(NULL), name_(name), flags_(FLAG_FORCE_STYLESHEET),
    expire_time_delta_(300), cache_time_(CACHE_TIME_UNDEFINED), page_random_max_(0) {
}

Script::ScriptData::~ScriptData() {
    std::for_each(blocks_.begin(), blocks_.end(), boost::checked_deleter<Block>());
}

bool
Script::ScriptData::threaded() const {
    return flags_ & FLAG_THREADED;
}

bool
Script::ScriptData::forceStylesheet() const {
    return flags_ & FLAG_FORCE_STYLESHEET;
}

bool
Script::ScriptData::binaryPage() const {
    return flags_ & FLAG_BINARY_PAGE;
}

unsigned int
Script::ScriptData::expireTimeDelta() const {
    return expire_time_delta_;
}

time_t
Script::ScriptData::cacheTime() const {
    return cache_time_;
}

boost::int32_t
Script::ScriptData::pageRandomMax() const {
    return page_random_max_;
}

bool
Script::ScriptData::allowMethod(const std::string& value) const {
    if (!allow_methods_.empty() &&
        std::find(allow_methods_.begin(), allow_methods_.end(), value) == allow_methods_.end()) {
        return false;
    }

    return true;
}

bool
Script::ScriptData::cacheAllQuery() const {
    return cache_query_.empty();
}

bool
Script::ScriptData::cacheQueryParam(const std::string &value) const {
    if (!cache_query_.empty() &&
        cache_query_.find(value) == cache_query_.end()) {
        return false;
    }
    return true;
}

unsigned int
Script::ScriptData::blocksNumber() const {
    return blocks_.size();
}

Block*
Script::ScriptData::block(unsigned int n) const {
    return blocks_.at(n);
}

Block*
Script::ScriptData::block(const std::string &id, bool throw_error) const {
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

std::vector<Block*>&
Script::ScriptData::blocks() const {
    return const_cast<std::vector<Block*>&>(blocks_);
}

std::set<std::string>&
Script::ScriptData::cacheCookies() const {
    return const_cast<std::set<std::string>&>(cache_cookies_);
}

const std::string&
Script::ScriptData::header(const std::string &name) const {
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

const std::map<std::string, std::string>&
Script::ScriptData::headers() const {
    return headers_;
}

void
Script::ScriptData::threaded(bool value) {
    flag(FLAG_THREADED, value);
}

void
Script::ScriptData::forceStylesheet(bool value) {
    flag(FLAG_FORCE_STYLESHEET, value);
}

void
Script::ScriptData::expireTimeDelta(unsigned int value) {
    expire_time_delta_ = value;
}

void
Script::ScriptData::cacheTime(time_t value) {
    cache_time_ = value;
}

void
Script::ScriptData::pageRandomMax(boost::int32_t value) {
    if (value < 0) {
        throw std::runtime_error("negative page random max is not allowed");
    }
    
    if (value > RAND_MAX) {
        throw std::runtime_error("page random max exceeded maximum allowed value");
    }
    
    page_random_max_ = value;
}

void
Script::ScriptData::binaryPage(bool value) {
    flag(FLAG_BINARY_PAGE, value);
}

void
Script::ScriptData::flag(unsigned int type, bool value) {
    flags_ = value ? (flags_ | type) : (flags_ &= ~type);
}

void
Script::ScriptData::addBlock(Block *block) {
    blocks_.push_back(block);
}

void
Script::ScriptData::addXscriptNode(xmlNodePtr node) {
    xscript_node_set_.insert(node);
}

void
Script::ScriptData::extensionProperty(const std::string &name, const std::string &value) {
    extension_properties_.insert(std::make_pair(name, value));
}

void
Script::ScriptData::addHeader(const std::string &name, const std::string &value) {
    headers_.insert(std::make_pair<std::string, std::string>(name, value));
}

void
Script::ScriptData::setHeaders(Response *response) {
    for (std::map<std::string, std::string>::const_iterator i = headers_.begin(), end = headers_.end(); i != end; ++i) {
        response->setHeader(i->first, i->second);
    }
}

const std::string&
Script::ScriptData::extensionProperty(const std::string &name) const {
    std::map<std::string, std::string, StringCILess>::const_iterator it =
        extension_properties_.find(name);
    
    if (it == extension_properties_.end()) {
        return StringUtils::EMPTY_STRING;
    }
    
    return it->second;
}

bool
Script::ScriptData::cacheTimeUndefined() const {
    return cache_time_ == CACHE_TIME_UNDEFINED;
}

std::set<xmlNodePtr>&
Script::ScriptData::xscriptNodes() const {
    return const_cast<std::set<xmlNodePtr>&>(xscript_node_set_);
}

const std::string&
Script::ScriptData::name() const {
    return name_;
}

void
Script::ScriptData::allowMethods(const char *value) {
    allow_methods_.clear();
    typedef boost::char_separator<char> Separator;
    typedef boost::tokenizer<Separator> Tokenizer;
    std::string method_list(value);
    Tokenizer tok(method_list, Separator(" "));
    for (Tokenizer::iterator it = tok.begin(), it_end = tok.end(); it != it_end; ++it) {
        allow_methods_.push_back(*it);
        std::vector<std::string>::reverse_iterator method = allow_methods_.rbegin();
        std::transform(method->begin(), method->end(), method->begin(),
                       boost::bind(&toupper, _1));
    }
}

void
Script::ScriptData::cacheQuery(const char *value) {
    cache_query_.clear();
    typedef boost::char_separator<char> Separator;
    typedef boost::tokenizer<Separator> Tokenizer;
    std::string param_list(value);
    Tokenizer tok(param_list, Separator(" "));
    for (Tokenizer::iterator it = tok.begin(), it_end = tok.end(); it != it_end; ++it) {
        cache_query_.insert(*it);
    }
    if (cache_query_.empty()) {
        cache_query_.insert(StringUtils::EMPTY_STRING);
    }
}

void
Script::ScriptData::cacheCookies(const char *value) {
    cache_cookies_.clear();
    typedef boost::char_separator<char> Separator;
    typedef boost::tokenizer<Separator> Tokenizer;
    std::string param_list(value);
    Tokenizer tok(param_list, Separator(", "));
    Policy *policy = Policy::instance();
    for (Tokenizer::iterator it = tok.begin(), it_end = tok.end(); it != it_end; ++it) {
        if (!policy->allowCachingInputCookie(it->c_str())) {
            std::stringstream ss;
            ss << "Cookie " << *it << " is not allowed in cache-cookies";
            throw std::runtime_error(ss.str());
        }
        cache_cookies_.insert(*it);
    }
}

Script::Script(const std::string &name) :
    Xml(), data_(new ScriptData(name))
{}

Script::~Script() {
    delete data_;
}

bool
Script::threaded() const {
    return data_->threaded();
}

bool
Script::forceStylesheet() const {
    return data_->forceStylesheet();
}

bool
Script::binaryPage() const {
    return data_->binaryPage();
}

unsigned int
Script::expireTimeDelta() const {
    return data_->expireTimeDelta();
}

time_t
Script::cacheTime() const {
    return data_->cacheTime();
}

boost::int32_t
Script::pageRandomMax() const {
    return data_->pageRandomMax();
}

bool
Script::allowMethod(const std::string& value) const {
    return data_->allowMethod(value);
}

bool
Script::cacheAllQuery() const {
    return data_->cacheAllQuery();
}

bool
Script::cacheQueryParam(const std::string &value) const {
    return data_->cacheQueryParam(value);
}

unsigned int
Script::blocksNumber() const {
    return data_->blocksNumber();
}

const Block*
Script::block(unsigned int n) const {
    return data_->block(n);
}

const Block*
Script::block(const std::string &id, bool throw_error) const {
    return data_->block(id, throw_error);
}

const std::vector<Block*>&
Script::blocks() const {
    return data_->blocks();
}

const std::string&
Script::header(const std::string &name) const {
    return data_->header(name);
}

const std::map<std::string, std::string>&
Script::headers() const {
    return data_->headers();
}

void
Script::threaded(bool value) {
    data_->threaded(value);
}

void
Script::forceStylesheet(bool value) {
    data_->forceStylesheet(value);
}

void
Script::expireTimeDelta(unsigned int value) {
    data_->expireTimeDelta(value);
}

void
Script::cacheTime(time_t value) {
    data_->cacheTime(value);
}    

void
Script::pageRandomMax(boost::int32_t value) {
    return data_->pageRandomMax(value);
}

void
Script::binaryPage(bool value) {
    data_->binaryPage(value);
}

void
Script::allowMethods(const char *value) {
    data_->allowMethods(value);
}

void
Script::cacheQuery(const char *value) {
    data_->cacheQuery(value);
}

void
Script::cacheCookies(const char *value) {
    data_->cacheCookies(value);
}

void
Script::flag(unsigned int type, bool value) {
    data_->flag(type, value);
}

void
Script::extensionProperty(const char *prop, const char *value) {
    data_->extensionProperty(prop, value);
}

void
Script::parse() {
    parse(StringUtils::EMPTY_STRING);
}

void
Script::parse(const std::string &xml) {

    XmlCharHelper canonic_path;
    bool read_from_disk = xml.empty();
    if (read_from_disk) {
        namespace fs = boost::filesystem;
        fs::path path(name());
        if (!fs::exists(path) || fs::is_directory(path)) {
            std::stringstream stream;
            stream << "can not open " << path.native_file_string();
            throw std::runtime_error(stream.str());
        }
        canonic_path = XmlCharHelper(xmlCanonicPath((const xmlChar *) path.native_file_string().c_str()));
    }

    XmlDocHelper doc(NULL);
    {
        XmlInfoCollector::Starter starter;
        if (!read_from_disk) {
            doc = XmlDocHelper(xmlReadMemory(xml.c_str(), xml.size(), StringUtils::EMPTY_STRING.c_str(),
                                    NULL, XML_PARSE_DTDATTR | XML_PARSE_NOENT));
        }
        else {
            doc = XmlDocHelper(xmlReadFile((const char*) canonic_path.get(),
                                    NULL, XML_PARSE_DTDATTR | XML_PARSE_NOENT));
        }

        XmlUtils::throwUnless(NULL != doc.get());
        if (NULL == doc->children) {
            throw std::runtime_error("got empty xml doc");
        }

        XmlUtils::throwUnless(xmlXIncludeProcessFlags(doc.get(), XML_PARSE_NOENT) >= 0);
        
        TimeMapType* modified_info = XmlInfoCollector::getModifiedInfo();
        TimeMapType fake;
        modified_info ? swapModifiedInfo(*modified_info) : swapModifiedInfo(fake);
        
        std::string error = XmlInfoCollector::getError();
        if (!error.empty()) {
            throw UnboundRuntimeError(error);
        }
    }

    std::vector<xmlNodePtr> xscript_nodes;
    parseNode(doc->children, xscript_nodes);
    parseXScriptNodes(xscript_nodes);
    parseBlocks();
    buildXScriptNodeSet(xscript_nodes);
    postParse();
    data_->doc_ = doc;
}

const std::string&
Script::name() const {
    return data_->name();
}

std::string
Script::fullName(const std::string &name) const {
    return Xml::fullName(name);
}

void
Script::removeUnusedNodes(const XmlDocHelper &doc) {
    (void)doc;
}

XmlDocHelper
Script::invoke(boost::shared_ptr<Context> ctx) {

    log()->info("%s, invoking %s", BOOST_CURRENT_FUNCTION, name().c_str());
    PROFILER(log(), "invoke script " + name());

    unsigned int count = 0;
    int to = countTimeout();
    
    std::vector<Block*> &blocks = data_->blocks();
    ctx->expect(blocks.size());
    
    for(std::vector<Block*>::iterator it = blocks.begin();
        it != blocks.end();
        ++it, ++count) {
        (*it)->invokeCheckThreaded(ctx, count);
    }

    ctx->wait(to);
    log()->debug("%s, finished to wait", BOOST_CURRENT_FUNCTION);
    
    OperationMode::instance()->processScriptError(ctx.get(), this);
    
    addHeaders(ctx.get());
    return fetchResults(ctx.get());
}

void
Script::applyStylesheet(boost::shared_ptr<Context> ctx, XmlDocHelper &doc) {    
    std::string xslt = ctx->xsltName();
    if (xslt.empty()) {
        return;
    }
    
    boost::shared_ptr<Stylesheet> stylesheet(Stylesheet::create(xslt));
    
    PROFILER(log(), "apply stylesheet " + name());
    log()->info("applying stylesheet to %s", name().c_str());
    
    ctx->createDocumentWriter(stylesheet);
    Object::applyStylesheet(stylesheet, ctx, doc, false);
    
    OperationMode::instance()->processMainXsltError(ctx.get(), this, stylesheet.get());
}

boost::shared_ptr<Script>
Script::create(const std::string &name) {
    return Script::create(name, StringUtils::EMPTY_STRING);
}

boost::shared_ptr<Script>
Script::create(const std::string &name, const std::string &xml) {

    if (!xml.empty()) {
        return createWithParse(name, xml);
    }
    
    ScriptCache *cache = ScriptCache::instance();
    boost::shared_ptr<Script> script = cache->fetch(name);
    if (NULL != script.get()) {
        return script;
    }
  
    boost::mutex *mutex = cache->getMutex(name);
    if (NULL == mutex) {
        return createWithParse(name, xml);
    }
    
    boost::mutex::scoped_lock lock(*mutex);
    script = cache->fetch(name);
    if (NULL != script.get()) {
        return script;
    }

    return createWithParse(name, xml);
}

boost::shared_ptr<Script>
Script::createWithParse(const std::string &name, const std::string &xml) {
    boost::shared_ptr<Script> script = ScriptFactory::instance()->create(name);
    script->parse(xml);
    if (xml.empty()) {
        ScriptCache::instance()->store(name, script);
    }
    return script;
}

void
Script::parseNode(xmlNodePtr node, std::vector<xmlNodePtr>& xscript_nodes) {
    ExtensionList* elist = ExtensionList::instance();
    while (NULL != node) {
        if (XML_PI_NODE == node->type) {
            if (xmlStrncasecmp(node->name, (const xmlChar*) "xml-stylesheet", sizeof("xml-stylesheet")) == 0) {
                if (xsltName().empty()) {
                    log()->debug("%s, parse stylesheet", name().c_str());
                    parseStylesheetNode(node);
                }
                else {
                    log()->debug("%s, skip stylesheet", name().c_str());
                }
                xmlNodePtr snode = node;
                node = node->next;
                xmlUnlinkNode(snode);
                xmlFreeNode(snode);
            }
            else {
                node = node->next;
            }
            continue;
        }
        if (XML_ELEMENT_NODE == node->type) {
            if (xmlStrncasecmp(node->name, (const xmlChar*) "xscript", sizeof("xscript")) == 0) {
                xscript_nodes.push_back(node);
                node = node->next;
                continue;
            }
            if (node->ns && node->ns->href &&
                xmlStrEqual(node->name, XINCLUDE_NODE) &&
                (xmlStrEqual(node->ns->href, XINCLUDE_NS) || xmlStrEqual(node->ns->href, XINCLUDE_OLD_NS))) {
                const char *href = XmlUtils::attrValue(node, "href");
                if (NULL != href) {
                    throw UnboundRuntimeError(
                        std::string("Cannot include file: ") + href +
                            ". Check include file for syntax error");
                }
            }            
            Extension *ext = elist->extension(node, true);
            if (NULL != ext) {
                log()->debug("%s, creating block %s", name().c_str(), ext->name());
                std::auto_ptr<Block> b = ext->createBlock(this, node);

                assert(b.get());

                data_->addBlock(b.get());
                b.release();
                node = node->next;
                continue;
            }
        }
        if (node->children) {
            parseNode(node->children, xscript_nodes);
        }
        node = node->next;
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
        if (child->name &&
            xmlStrncasecmp(child->name, (const xmlChar*) "add-headers", sizeof("add-headers")) == 0) {
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
    std::vector<Block*> &blocks = data_->blocks();
    for(std::vector<Block*>::iterator it = blocks.begin();
        it != blocks.end();
        ++it) {
        Block *block = *it;
        assert(block);
        block->threaded(is_threaded);
        block->parse();
    }
}

void
Script::buildXScriptNodeSet(std::vector<xmlNodePtr>& xscript_nodes) {

    log()->debug("build xscript node set");

    for (std::vector<xmlNodePtr>::iterator i = xscript_nodes.begin(), end = xscript_nodes.end(); i != end; ++i) {
        data_->addXscriptNode(*i);
    }
    xscript_nodes.clear();
}

void
Script::parseHeadersNode(xmlNodePtr node) {
    while (node) {
        if (node->name &&
            xmlStrncasecmp(node->name, (const xmlChar*) "header", sizeof("header")) == 0) {
            const char *name = XmlUtils::attrValue(node, "name"), *value = XmlUtils::attrValue(node, "value");
            if (name && value) {
                data_->addHeader(name, value);
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
                    if (begin == end) {
                        throw std::runtime_error("empty href in stylesheet node");
                    }
                    xsltName(std::string((const char*) begin, (const char*) end));
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
    std::vector<Block*> &blocks = data_->blocks();
    for(std::vector<Block*>::iterator it = blocks.begin();
        it != blocks.end();
        ++it) {
        const ThreadedBlock *tb = dynamic_cast<const ThreadedBlock*>(*it);
        log()->debug("%s, is block threaded: %d", BOOST_CURRENT_FUNCTION, static_cast<int>(NULL != tb));
        if (tb && tb->threaded()) {
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
    addExpiresHeader(ctx);
    data_->setHeaders(response);
}

void
Script::addExpiresHeader(const Context *ctx) const {
    ctx->response()->setHeader(
            "Expires", HttpDateUtils::format(time(NULL) + expireTimeDelta()));
}

XmlDocHelper
Script::fetchResults(Context *ctx) const {

    XmlDocHelper newdoc(xmlCopyDoc(data_->doc_.get(), 1));
    XmlUtils::throwUnless(NULL != newdoc.get());

    unsigned int count = 0, xscript_count = 0;

    xmlNodePtr node = xmlDocGetRootElement(data_->doc_.get());
    assert(node);
    
    xmlNodePtr newnode = xmlDocGetRootElement(newdoc.get());
    assert(newnode);
    
    fetchRecursive(ctx, node, newnode, count, xscript_count);
    return newdoc;
}

void
Script::fetchRecursive(Context *ctx, xmlNodePtr node, xmlNodePtr newnode,
                       unsigned int &count, unsigned int &xscript_count) const {

    const std::set<xmlNodePtr>& xscript_node_set = data_->xscriptNodes();
    unsigned int blocks_num = blocksNumber();
    
    while (node && count + xscript_count != blocks_num + xscript_node_set.size()) {
        if (newnode == NULL) {
            throw std::runtime_error(std::string("internal error in node ") + (char*)node->name);
        }
        xmlNodePtr next = newnode->next;
        log()->debug("%s, blocks found: %d, %u", BOOST_CURRENT_FUNCTION, blocks_num, count);
        if (count < blocks_num && block(count)->node() == node) {
            InvokeResult result = ctx->result(count);
            xmlDocPtr doc = result.doc.get();
            assert(doc);

            xmlNodePtr result_doc_root_node = xmlDocGetRootElement(doc);
            if (result_doc_root_node) {
                const Block *blck = block(count);
                if (blck->xpointer(ctx) && !result.error()) {
                    const std::string &expression = blck->xpointerExpr();
                    if ("/.." == expression) {
                        xmlUnlinkNode(newnode);
                    }
                    else {
                        try {
                            useXpointerExpr(doc, newnode, (xmlChar *)expression.c_str());
                        }
                        catch (std::exception &e) {
                            std::string message = "XPointer error with expression " + expression + " : ";
                            message.append(e.what());
                            throw std::runtime_error(message);
                        }
                    }
                }
                else {
                    xmlReplaceNode(newnode, xmlCopyNode(result_doc_root_node, 1));
                }
            }
            else {
                xmlUnlinkNode(newnode);
            }
            xmlFreeNode(newnode);
            count++;
        }
        else if (xscript_node_set.find(node) != xscript_node_set.end() ) {
            replaceXScriptNode(node, newnode, ctx);
            xscript_count++;
        }
        else if (node->children) {
            fetchRecursive(ctx, node->children, newnode->children, count, xscript_count);
        }

        node = node->next;
        newnode = next;
    }
}

void
Script::useXpointerExpr(xmlDocPtr doc, xmlNodePtr newnode, xmlChar *xpath) const {
    XmlXPathContextHelper context(xmlXPathNewContext(doc));
    XmlUtils::throwUnless(NULL != context.get());
    XmlXPathObjectHelper xpathObj(xmlXPathEvalExpression(xpath, context.get()));
    XmlUtils::throwUnless(NULL != xpathObj.get());

    if (XPATH_BOOLEAN == xpathObj->type) {
        const char *str = 0 != xpathObj->boolval ? "1" : "0";
        xmlNodePtr text_node = xmlNewText((const xmlChar *)str);
        xmlReplaceNode(newnode, text_node);
        return;
    }
    if (XPATH_NUMBER == xpathObj->type) {
        char str[40];
        snprintf(str, sizeof(str) - 1, "%f", xpathObj->floatval);
        str[sizeof(str) - 1] = '\0';
        xmlNodePtr text_node = xmlNewText((const xmlChar *)&str[0]);
        xmlReplaceNode(newnode, text_node);
        return;
    }
    if (XPATH_STRING == xpathObj->type) {
        xmlNodePtr text_node = xmlNewText((const xmlChar *)xpathObj->stringval);
        xmlReplaceNode(newnode, text_node);
        return;
    }

    xmlNodeSetPtr nodeset = xpathObj->nodesetval;
    if (NULL == nodeset || 0 == nodeset->nodeNr) {
        xmlUnlinkNode(newnode);
        return;
    }
    
    xmlNodePtr current_node = nodeset->nodeTab[0];    
    if (XML_ATTRIBUTE_NODE == current_node->type) {
        current_node = current_node->children;
    }    
    xmlNodePtr last_input_node = xmlCopyNode(current_node, 1);         
    xmlReplaceNode(newnode, last_input_node);
    for (int i = 1; i < nodeset->nodeNr; ++i) {
        xmlNodePtr current_node = nodeset->nodeTab[i];    
        if (XML_ATTRIBUTE_NODE == current_node->type) {
            current_node = current_node->children;
        }                
        xmlNodePtr insert_node = xmlCopyNode(current_node, 1);         
        xmlAddNextSibling(last_input_node, insert_node);
        last_input_node = insert_node;
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
    else if (strncasecmp(prop, "allow-methods", sizeof("allow-methods")) == 0) {
        allowMethods(value);
    }
    else if (strncasecmp(prop, "xslt-dont-apply", sizeof("xslt-dont-apply")) == 0) {
        forceStylesheet(strncasecmp(value, "yes", sizeof("yes")) != 0);
    }
    else if (strncasecmp(prop, "http-expire-time-delta", sizeof("http-expire-time-delta")) == 0) {
        expireTimeDelta(boost::lexical_cast<unsigned int>(value));
    }
    else if (strncasecmp(prop, "cache-time", sizeof("cache-time")) == 0) {
        cacheTime(boost::lexical_cast<time_t>(value));
    }
    else if (strncasecmp(prop, "binary-page", sizeof("binary-page")) == 0) {
        binaryPage(strncasecmp(value, "yes", sizeof("yes")) == 0);
    }
    else if (strncasecmp(prop, "page-random-max", sizeof("page-random-max")) == 0) {
        pageRandomMax(boost::lexical_cast<boost::int32_t>(value));
    }
    else if (strncasecmp(prop, "cache-query", sizeof("cache-query")) == 0) {
        cacheQuery(value);
    }
    else if (strncasecmp(prop, "cache-cookies", sizeof("cache-cookies")) == 0) {
        cacheCookies(value);
    }
    else if (ExtensionList::instance()->checkScriptProperty(prop, value)) {
        extensionProperty(prop, value);
    }
    else {
        throw std::runtime_error(std::string("invalid script property: ").append(prop));
    }
}

void
Script::replaceXScriptNode(xmlNodePtr node, xmlNodePtr newnode, Context *ctx) const {
    (void)ctx;
    (void)node;
    xmlUnlinkNode(newnode);
    xmlFreeNode(newnode);
}

const std::string&
Script::extensionProperty(const std::string &name) const {
    return data_->extensionProperty(name);
}

bool
Script::cacheTimeUndefined() const {
    return data_->cacheTimeUndefined();
}

std::string
Script::cachedUrl(const Context *ctx) const {
    std::string key(ctx->request()->getOriginalUrl());
    if (!cacheAllQuery()) {
        std::string::size_type pos = key.rfind('?');
        if (std::string::npos != pos) {
            key.erase(pos);
        }
        
        Request *request = ctx->request();
        
        std::vector<std::string> names;
        request->argNames(names);
        
        bool first_param = true;
        for (std::vector<std::string>::const_iterator i = names.begin(), end = names.end(); i != end; ++i) {
            std::string name = *i;
            std::vector<std::string> values;
            if (name.empty() || !cacheQueryParam(name)) {
                continue;
            }
            request->getArg(name, values);
            for (std::vector<std::string>::const_iterator it = values.begin(), end = values.end(); it != end; ++it) {
                if (first_param) {
                    key.push_back('?');
                    first_param = false;
                }
                else {
                    key.push_back('&');
                }
                key.append(name);
                key.push_back('=');
                key.append(*it);
            }
        }
    }
    return key;
}

std::string
Script::createTagKey(const Context *ctx) const {
    
    std::string key = cachedUrl(ctx);
    key.push_back('|');

    const TimeMapType& modified_info = modifiedInfo();
    for(TimeMapType::const_iterator it = modified_info.begin();
        it != modified_info.end();
        ++it) {
        key.append(boost::lexical_cast<std::string>(it->second));
        key.push_back('|');
    }
    
    const std::string &xslt = xsltName();
    if (!xslt.empty()) {
        namespace fs = boost::filesystem;
        fs::path path(xslt);
        if (fs::exists(path) && !fs::is_directory(path)) {
            key.append(boost::lexical_cast<std::string>(fs::last_write_time(path)));
            key.push_back('|');
        }
        else {
            throw std::runtime_error("Cannot stat stylesheet " + xslt);
        }
    }
    
    std::vector<Block*> &blocks = data_->blocks();
    for(std::vector<Block*>::iterator it = blocks.begin();
        it != blocks.end();
        ++it) {
        Block *block = *it;
        const std::string& xslt_name = block->xsltName();
        if (!xslt_name.empty()) {              
            namespace fs = boost::filesystem;
            fs::path path(xslt_name);
            if (fs::exists(path) && !fs::is_directory(path)) {
                key.append(boost::lexical_cast<std::string>(fs::last_write_time(path)));
                key.push_back('|');
            }
            else {
                throw std::runtime_error("Cannot stat stylesheet " + xslt_name);
            }                
        }            
    }
    
    std::set<std::string> &cache_cookies = data_->cacheCookies();
    for(std::set<std::string>::iterator it = cache_cookies.begin();
        it != cache_cookies.end();
        ++it) {
        if (ctx->request()->hasCookie(*it)) {
            key.push_back('|');
            key.append(*it);
            key.push_back(':');
            key.append(ctx->request()->getCookie(*it));
        }
    }
    
    const std::string &ctx_key = ctx->key();
    if (!ctx_key.empty()) {
        key.push_back('|');
        key.append(ctx_key);
    }
    
    return key;
}

std::string
Script::info(const Context *ctx) const {
    std::string info("Original url: ");
    info.append(cachedUrl(ctx));
    info.append(" | Filename: ");
    info.append(name());
    if (!cacheTimeUndefined()) {
        info.append(" | Cache-time: ");
        info.append(boost::lexical_cast<std::string>(cacheTime()));
    }
    
    const std::string& ctx_key = ctx->key();
    if (!ctx_key.empty()) {
        info.append(" | Page key: ").append(ctx_key);
    }
    
    return info;
}

bool
Script::cachable(const Context *ctx, bool for_save) const {
    
    if (ctx->noCache() || binaryPage() ||
        ctx->request()->getRequestMethod() != GET_METHOD) {
        return false;
    }
    
    if (cacheTimeUndefined() || cacheTime() < DocCache::instance()->minimalCacheTime()) {
        return false;
    }
    
    if (for_save) {
        if (ctx->xsltChanged(this) || !ctx->response()->isStatusOK()) {
            return false;
        }
        
        const CookieSet &cookies = ctx->response()->outCookies();       
        Policy *policy = Policy::instance();
        for(CookieSet::const_iterator it = cookies.begin(); it != cookies.end(); ++it) {
            if (!policy->allowCachingOutputCookie(it->name().c_str())) {
                return false;
            }
        }
    }
    
    return true;
}

} // namespace xscript
