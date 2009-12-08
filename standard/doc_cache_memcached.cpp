#include <memcache.h>

#include "xscript/doc_cache.h"
#include "xscript/string_utils.h"
#include "xscript/tag.h"
#include "xscript/util.h"
#include "xscript/xml_util.h"
#include "tag_key_memory.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

/**
 * TagKey for memcached. Hashes value from TagKeyMemory
 */
class TagKeyMemcached : public TagKeyMemory {
public:
    TagKeyMemcached(const Context *ctx, const CachedObject *obj)
        : TagKeyMemory(ctx, obj)
    {
        value_ = HashUtils::hexMD5(value_.c_str(), value_.length());
    }
};

/**
 * Implementation of DocCacheStrategy using memcached.
 */
class DocCacheMemcached :
            public Component<DocCacheMemcached>,
            public DocCacheStrategy {
public:
    DocCacheMemcached();
    virtual ~DocCacheMemcached();

    virtual void init(const Config *config);

    virtual time_t minimalCacheTime() const;
    virtual std::string name() const;

    virtual std::auto_ptr<TagKey> createKey(const Context *ctx, const CachedObject *obj) const;
    
    virtual bool distributed() const;
    
protected:
    virtual bool loadDocImpl(const TagKey *key, Tag &tag, XmlDocSharedHelper &doc, bool need_copy);
    virtual bool saveDocImpl(const TagKey *key, const Tag& tag, const XmlDocSharedHelper &doc, bool need_copy);

private:
    struct memcached_st *mc_;
    boost::uint32_t max_size_;
};

DocCacheMemcached::DocCacheMemcached() : max_size_(0)
{
    mc_ = memcached_create(NULL);
    if (!mc_) 
        throw std::runtime_error("Unable to allocate new memcache object");

    CacheStrategyCollector::instance()->addStrategy(this, name());
}

DocCacheMemcached::~DocCacheMemcached()
{
    memcached_free(mc_);
}


bool
DocCacheMemcached::distributed() const {
    return true;
}

void
DocCacheMemcached::init(const Config *config) {
    log()->debug("initing Memcached");
    DocCacheStrategy::init(config);
    
    Config::addForbiddenKey("/xscript/tagged-cache-memcached/*");
    
    std::vector<std::string> names;
    config->subKeys(std::string("/xscript/tagged-cache-memcached/server"), names);
    
    max_size_ = config->as<boost::uint32_t>("/xscript/tagged-cache-memcached/max-size", 1048497);
    
    if (names.empty()) {
        throw std::runtime_error("No memcached servers specified in config");
    }

    for (std::vector<std::string>::iterator i = names.begin(), end = names.end(); i != end; ++i) {
        std::string server = config->as<std::string>(*i);
        log()->debug("Adding %s", server.c_str());
        mc_server_add4(mc_, server.c_str());
        
        memcached_server_add
    }
    
    std::string no_cache =
        config->as<std::string>("/xscript/tagged-cache-memcached/no-cache", StringUtils::EMPTY_STRING);

    insert2Cache(no_cache);
}
    
time_t 
DocCacheMemcached::minimalCacheTime() const {
    return 5;
}

std::string
DocCacheMemcached::name() const {
    return "memcached";
}

std::auto_ptr<TagKey> 
DocCacheMemcached::createKey(const Context *ctx, const CachedObject *obj) const {
    log()->debug("Creating key");
    return std::auto_ptr<TagKey>(new TagKeyMemcached(ctx, obj));
}

extern "C" int
memcacheWriteFunc(void *ctx, const char *data, int len) {
    std::string * str = static_cast<std::string*>(ctx);
    str->append(data, len);
    return len;
}

extern "C" int
memcacheCloseFunc(void *ctx) {
    (void) ctx;
    return 0;
}

bool 
DocCacheMemcached::saveDocImpl(const TagKey *key, const Tag& tag, const XmlDocSharedHelper &doc, bool need_copy) {
    (void)need_copy;
    log()->debug("saving doc in memcached");
  
    std::string mc_key = key->asString();
    std::string val;

    // Adding Tag
    val.append((char*)&tag.last_modified, sizeof(tag.last_modified));
    val.append((char*)&tag.expire_time, sizeof(tag.expire_time));

    xmlOutputBufferPtr buf = NULL;
    buf = xmlOutputBufferCreateIO(&memcacheWriteFunc, &memcacheCloseFunc, &val, NULL);

    xmlSaveFormatFileTo(buf, doc->get(), "UTF-8", 0);

    boost::uint32_t size = val.length();
    if (size > max_size_) {
        log()->info("object size %d exceeds limit %d", size, max_size_);
        return false;
    }
    
    char * mc_key2 = strdup(mc_key.c_str());
    mc_set(mc_, mc_key2, mc_key.length(), val.c_str(), val.length(), tag.expire_time, 0);
    free(mc_key2);
    return true;
}

bool 
DocCacheMemcached::loadDocImpl(const TagKey *key, Tag &tag, XmlDocSharedHelper &doc, bool need_copy) {
    (void)need_copy;
    log()->debug("loading doc in memcached");

    std::string mc_key = key->asString();
    char * mc_key2 = strdup(mc_key.c_str());
    if (!mc_key2) {
        log()->warn("Cannot copy key for memcached");
        return false;
    }
    
    size_t vallen;
    char * val = static_cast<char*>(mc_aget2(mc_, mc_key2, mc_key.length(), &vallen));
    free(mc_key2);

    if (!val) {
        return false;
    }

    log()->debug("Loaded! %s", val);
    try {
        log()->debug("Parsing");

        char * orig_val = val;

        // Setting Tag
        tag.last_modified = *((time_t*)(val));
        val += sizeof(time_t);
        tag.expire_time = *((time_t*)(val));
        val += sizeof(time_t);
        vallen -= 2*sizeof(time_t);

        doc = XmlDocSharedHelper(new XmlDocHelper(xmlParseMemory(val, vallen)));
        log()->debug("Parsed %p", doc.get());
        free(orig_val);
        XmlUtils::throwUnless(NULL != doc.get());
        return true;
    }
    catch (const std::exception &e) {
        log()->error("error while parsing doc from cache: %s", e.what());
        return false;
    }
}

static ComponentRegisterer<DocCacheMemcached> reg_;

};
