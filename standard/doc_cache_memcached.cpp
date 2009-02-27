#include <memcache.h>

#include "xscript/doc_cache.h"
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
    TagKeyMemcached(const Context *ctx, const TaggedBlock *block)
        : TagKeyMemory(ctx, block)
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

    virtual std::auto_ptr<TagKey> createKey(const Context *ctx, const TaggedBlock *block) const;
protected:
    virtual bool loadDocImpl(const TagKey *key, Tag &tag, XmlDocHelper &doc);
    virtual bool saveDocImpl(const TagKey *key, const Tag& tag, const XmlDocHelper &doc);

private:
    struct memcache *mc_;
};

DocCacheMemcached::DocCacheMemcached()
{
    mc_ = mc_new();
    if (!mc_) 
        throw std::runtime_error("Unable to allocate new memcache object");

    statBuilder_.setName("tagged-cache-memcached");
    DocCache::instance()->addStrategy(this, "memcached");
}

DocCacheMemcached::~DocCacheMemcached()
{
    mc_free(mc_);
}


void
DocCacheMemcached::init(const Config *config) {
    log()->debug("initing Memcached");
    
    std::vector<std::string> names;
    config->subKeys(std::string("/xscript/tagged-cache-memcached/server"), names);
    if (names.empty()) {
        throw std::runtime_error("No memcached servers specified in config");
    }

    for (std::vector<std::string>::iterator i = names.begin(), end = names.end(); i != end; ++i) {
        std::string server = config->as<std::string>(*i);
        log()->debug("Adding %s", server.c_str());
        mc_server_add4(mc_, server.c_str());
    }
}
    
time_t 
DocCacheMemcached::minimalCacheTime() const {
    return 5;
}

std::auto_ptr<TagKey> 
DocCacheMemcached::createKey(const Context *ctx, const TaggedBlock *block) const {
    log()->debug("Creating key");
    return std::auto_ptr<TagKey>(new TagKeyMemcached(ctx, block));
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
DocCacheMemcached::saveDocImpl(const TagKey *key, const Tag& tag, const XmlDocHelper &doc) {
    log()->debug("saving doc");
  
    std::string mc_key = key->asString();
    std::string val;

    // Adding Tag
    val.append((char*)&tag.last_modified, sizeof(tag.last_modified));
    val.append((char*)&tag.expire_time, sizeof(tag.expire_time));

    xmlOutputBufferPtr buf = NULL;
    buf = xmlOutputBufferCreateIO(&memcacheWriteFunc, &memcacheCloseFunc, &val, NULL);

    xmlSaveFormatFileTo(buf, doc.get(), "UTF-8", 0);

    char * mc_key2 = strdup(mc_key.c_str());
    mc_set(mc_, mc_key2, mc_key.length(), val.c_str(), val.length(), tag.expire_time, 0);
    free(mc_key2);
    return true;
}

bool 
DocCacheMemcached::loadDocImpl(const TagKey *key, Tag &tag, XmlDocHelper &doc) {
    log()->debug("loading doc");

    std::string mc_key = key->asString();
    char * mc_key2 = strdup(mc_key.c_str());
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

        doc = XmlDocHelper(xmlParseMemory(val, vallen));
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
