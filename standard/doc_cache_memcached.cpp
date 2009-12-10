#include <libmemcached/memcached.h>

#include <boost/thread/tss.hpp>

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

class MemcachedProvider : private boost::noncopyable {
public:
    MemcachedProvider() {
        mc_ = memcached_create(NULL);
        if (!mc_) {
            throw std::runtime_error("Unable to allocate new memcached object");
        }
    }

    ~MemcachedProvider() {
        memcached_free(mc_);
    }
    
    memcached_st* get() {
        return mc_;
    }

private:
    memcached_st *mc_;
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
    virtual bool saveDocImpl(const TagKey *key, const Tag &tag, const XmlDocSharedHelper &doc, bool need_copy);
    void initMemcached();

private:
    boost::uint32_t max_size_;
    boost::int32_t timeout_;
    std::multimap<std::string, unsigned int> servers_;
    boost::thread_specific_ptr<MemcachedProvider> mc_;
};

DocCacheMemcached::DocCacheMemcached() : max_size_(0)
{
    CacheStrategyCollector::instance()->addStrategy(this, name());
}

DocCacheMemcached::~DocCacheMemcached()
{}


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
    timeout_ = config->as<boost::int32_t>("/xscript/tagged-cache-memcached/timeout", 0);
    
    if (names.empty()) {
        throw std::runtime_error("No memcached servers specified in config");
    }

    for (std::vector<std::string>::iterator i = names.begin(), end = names.end(); i != end; ++i) {
        std::string server = config->as<std::string>(*i);
        log()->debug("Adding %s", server.c_str());
        std::string host;
        unsigned int port = 0;
        std::string::size_type pos = server.find(":");
        if (pos != std::string::npos && server[pos + 1] != '\0') {
            try {
                port = boost::lexical_cast<unsigned int>(server.substr(pos + 1));
            }
            catch(const boost::bad_lexical_cast&) {
                log()->error("Cannot parse memcached server port: %s", server.c_str());
                continue;
            }
        }
        
        servers_.insert(std::make_pair(server.substr(0, pos), port));
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
DocCacheMemcached::saveDocImpl(const TagKey *key, const Tag &tag, const XmlDocSharedHelper &doc, bool need_copy) {
    (void)need_copy;
    log()->debug("saving doc in memcached");
  
    initMemcached();
    
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
    
    memcached_return rv = memcached_set(
        mc_->get(), mc_key.c_str(), mc_key.length(), val.c_str(), val.length(), tag.expire_time, 0);
    
    if (MEMCACHED_SUCCESS != rv) {
        log()->warn("Saving data to memcached failed: %d", rv);
        return false;
    }
    
    return true;
}

bool 
DocCacheMemcached::loadDocImpl(const TagKey *key, Tag &tag, XmlDocSharedHelper &doc, bool need_copy) {
    (void)need_copy;
    log()->debug("loading doc in memcached");

    initMemcached();
    
    std::string mc_key = key->asString();
    
    size_t vallen;
    uint32_t flags = 0;
    memcached_return rv;
    CharHelper val(memcached_get(mc_->get(), mc_key.c_str(), mc_key.length(), &vallen, &flags, &rv));

    if (NULL == val.get() || MEMCACHED_SUCCESS != rv) {
        if (MEMCACHED_NOTFOUND != rv) {
            log()->warn("Loading data from memcached failed: %d", rv);    
        }
        return false;
    }

    log()->debug("Loaded! %s", val.get());
    try {
        log()->debug("Parsing");

        char* value = val.get();

        tag.last_modified = *((time_t*)(value));
        value += sizeof(time_t);
        tag.expire_time = *((time_t*)(value));
        value += sizeof(time_t);
        vallen -= 2*sizeof(time_t);

        doc = XmlDocSharedHelper(new XmlDocHelper(xmlParseMemory(value, vallen)));
        log()->debug("Parsed %p", doc.get());
        XmlUtils::throwUnless(NULL != doc.get());
        return true;
    }
    catch (const std::exception &e) {
        log()->error("error while parsing doc from cache: %s", e.what());
        return false;
    }
}


void
DocCacheMemcached::initMemcached() {
    if (NULL == mc_.get()) {
        mc_.reset(new MemcachedProvider());
        
        int count = 0;
        for(std::multimap<std::string, unsigned int>::const_iterator it = servers_.begin();
            it != servers_.end();
            ++it) {
            if (MEMCACHED_SUCCESS != memcached_server_add(mc_->get(), it->first.c_str(), it->second)) {
                log()->error("Cannot add memcached server: %s:%d", it->first.c_str(), it->second);
            }
            else {
                ++count;
            }
        }
        
        if (count == 0) {
            mc_.reset();
            throw std::runtime_error("Cannot add any memcached server");
        }
        
        if (timeout_ > 0) {
            if (MEMCACHED_SUCCESS != memcached_behavior_set(mc_->get(), MEMCACHED_BEHAVIOR_POLL_TIMEOUT, timeout_)) {
                log()->error("Cannot set memcached poll timeout");
            }
            if (MEMCACHED_SUCCESS != memcached_behavior_set(mc_->get(), MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, timeout_)) {
                log()->error("Cannot set memcached connect timeout");
            }
            if (MEMCACHED_SUCCESS != memcached_behavior_set(mc_->get(), MEMCACHED_BEHAVIOR_SND_TIMEOUT, timeout_)) {
                log()->error("Cannot set memcached send timeout");
            }
            if (MEMCACHED_SUCCESS != memcached_behavior_set(mc_->get(), MEMCACHED_BEHAVIOR_RCV_TIMEOUT, timeout_)) {
                log()->error("Cannot set memcached receive timeout");
            }
        }
    }
}

static ComponentRegisterer<DocCacheMemcached> reg_;

};
