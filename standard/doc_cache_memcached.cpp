#include <list>

#include <libmemcached/memcached.h>

#include <boost/shared_ptr.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>

#include "xscript/cache_strategy_collector.h"
#include "xscript/doc_cache.h"
#include "xscript/http_utils.h"
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

class MemcachedPool;
class MemcachedConnection;

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
    
    virtual CachedObject::Strategy strategy() const;
    
    virtual bool loadDoc(const TagKey *key, CacheContext *cache_ctx,
        Tag &tag, boost::shared_ptr<CacheData> &cache_data);
    virtual bool saveDoc(const TagKey *key, CacheContext *cache_ctx,
        const Tag &tag, const boost::shared_ptr<CacheData> &cache_data);

private:
    boost::uint32_t max_size_;
    std::auto_ptr<MemcachedPool> pool_;
};

template<>
inline void ResourceHolderTraits<memcached_st*>::destroy(memcached_st *value) {
    memcached_free(value);
};

typedef ResourceHolder<memcached_st*> MemcahcedHelper;

struct MemcachedSetup {
    MemcachedSetup() : poll_timeout_(0), connect_timeout_(0), send_timeout_(0), recv_timeout_(0) {}
    std::multimap<std::string, boost::uint16_t> servers_;
    boost::uint16_t poll_timeout_;
    boost::uint16_t connect_timeout_;
    boost::uint16_t send_timeout_;
    boost::uint16_t recv_timeout_;
};

class MemcachedPool {
public:
    MemcachedPool(unsigned int size, const MemcachedSetup &setup);
    ~MemcachedPool();
    std::auto_ptr<MemcachedConnection> get();
    
    class MemcachedPoolImpl {
    public:
        MemcachedPoolImpl(unsigned int size, const MemcachedSetup &setup);
        ~MemcachedPoolImpl();
        memcached_st* get();
        
        friend class MemcachedConnection;
    private:
        void add(memcached_st *mc);
    private:
        boost::mutex mutex_;
        boost::condition condition_;
        std::list<memcached_st*> pool_;
    };
    
private:    
    boost::shared_ptr<MemcachedPoolImpl> impl_;
};

class MemcachedConnection : private boost::noncopyable {
public:
    MemcachedConnection(memcached_st *mc, boost::shared_ptr<MemcachedPool::MemcachedPoolImpl> pool) :
        mc_(mc), pool_(pool)
    {}

    ~MemcachedConnection() {
        pool_->add(mc_);
    }
    
    memcached_st* get() {
        return mc_;
    }

private:
    memcached_st *mc_;
    boost::shared_ptr<MemcachedPool::MemcachedPoolImpl> pool_;
};

MemcachedPool::MemcachedPool(unsigned int size, const MemcachedSetup &setup) :
        impl_(new MemcachedPoolImpl(size, setup))
{}

MemcachedPool::~MemcachedPool()
{}

std::auto_ptr<MemcachedConnection>
MemcachedPool::get() {
    return std::auto_ptr<MemcachedConnection>(new MemcachedConnection(impl_->get(), impl_));
}

MemcachedPool::MemcachedPoolImpl::MemcachedPoolImpl(unsigned int size, const MemcachedSetup &setup) {
    for(unsigned int i = 0; i < size; ++i) {
        MemcahcedHelper mc(memcached_create(NULL));
        if (!mc.get()) {
            throw std::runtime_error("Unable to allocate new memcached object");
        }
        
        int count = 0;
        for(std::multimap<std::string, boost::uint16_t>::const_iterator it = setup.servers_.begin();
            it != setup.servers_.end();
            ++it) {
            if (MEMCACHED_SUCCESS != memcached_server_add(mc.get(), it->first.c_str(), it->second)) {
                log()->error("Cannot add memcached server: %s:%d", it->first.c_str(), it->second);
            }
            else {
                ++count;
            }
        }
        
        if (count == 0) {
            throw std::runtime_error("Cannot add any memcached server");
        }
        
        if (setup.poll_timeout_ > 0) {
            if (MEMCACHED_SUCCESS != memcached_behavior_set(
                    mc.get(), MEMCACHED_BEHAVIOR_POLL_TIMEOUT, setup.poll_timeout_)) {
                log()->error("Cannot set memcached poll timeout");
            }
        }
        
        if (setup.connect_timeout_ > 0) {
            if (MEMCACHED_SUCCESS != memcached_behavior_set(
                    mc.get(), MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, setup.connect_timeout_)) {
                log()->error("Cannot set memcached connect timeout");
            }
        }

        if (setup.send_timeout_ > 0) {
            if (MEMCACHED_SUCCESS != memcached_behavior_set(
                    mc.get(), MEMCACHED_BEHAVIOR_SND_TIMEOUT, setup.send_timeout_)) {
                log()->error("Cannot set memcached send timeout");
            }
        }

        if (setup.recv_timeout_ > 0) {
            if (MEMCACHED_SUCCESS != memcached_behavior_set(
                    mc.get(), MEMCACHED_BEHAVIOR_RCV_TIMEOUT, setup.recv_timeout_)) {
                log()->error("Cannot set memcached receive timeout");
            }
        }
        
        pool_.push_back(mc.release());
    }
}

MemcachedPool::MemcachedPoolImpl::~MemcachedPoolImpl() {
    for(std::list<memcached_st*>::iterator it = pool_.begin();
        it != pool_.end();
        ++it) {
        memcached_free(*it);
    }
}

memcached_st*
MemcachedPool::MemcachedPoolImpl::get() {
    boost::mutex::scoped_lock lock(mutex_);
    if (pool_.empty()) {
        condition_.wait(lock);
    }
    memcached_st *mc = pool_.front();
    pool_.pop_front();
    return mc;
}
    
void
MemcachedPool::MemcachedPoolImpl::add(memcached_st *mc) {
    boost::mutex::scoped_lock lock(mutex_);
    pool_.push_back(mc);
    lock.unlock();
    condition_.notify_all();
}

DocCacheMemcached::DocCacheMemcached() : max_size_(0)
{
    CacheStrategyCollector::instance()->addStrategy(this, name());
}

DocCacheMemcached::~DocCacheMemcached()
{}

CachedObject::Strategy
DocCacheMemcached::strategy() const {
    return CachedObject::DISTRIBUTED;
}

void
DocCacheMemcached::init(const Config *config) {
    log()->debug("initing Memcached");
    DocCacheStrategy::init(config);
    
    Config::addForbiddenKey("/xscript/tagged-cache-memcached/*");
    
    std::string cache_strategy = config->as<std::string>(
            "/xscript/tagged-cache-memcached/enable", "");
    if (strcasecmp(cache_strategy.c_str(), "yes") == 0) {
    }
    else if (strcasecmp(cache_strategy.c_str(), "no") == 0) {
        CachedObject::clearDefaultStrategy(CachedObject::DISTRIBUTED);
    }
    else if (!cache_strategy.empty()) {
        throw std::runtime_error("Unknown cache strategy: " + cache_strategy);
    }
    
    std::vector<std::string> names;
    config->subKeys(std::string("/xscript/tagged-cache-memcached/server"), names);
    
    max_size_ = config->as<boost::uint32_t>("/xscript/tagged-cache-memcached/max-size", 1048497);
    boost::uint32_t workers = config->as<boost::uint32_t>("/xscript/tagged-cache-memcached/workers", 10);
    
    MemcachedSetup setup;
    setup.poll_timeout_ = config->as<boost::uint16_t>("/xscript/tagged-cache-memcached/poll-timeout", 100);
    setup.connect_timeout_ = config->as<boost::uint16_t>("/xscript/tagged-cache-memcached/connect-timeout", 100);
    setup.send_timeout_ = config->as<boost::uint16_t>("/xscript/tagged-cache-memcached/transfer-timeout", 1000);
    setup.recv_timeout_ = setup.send_timeout_;
    
    if (names.empty()) {
        throw std::runtime_error("No memcached servers specified in config");
    }

    for(std::vector<std::string>::iterator i = names.begin(), end = names.end(); i != end; ++i) {
        std::string server = config->as<std::string>(*i);
        log()->debug("Adding %s", server.c_str());
        std::string host;
        boost::uint16_t port = 0;
        std::string::size_type pos = server.find(":");
        if (pos != std::string::npos && server[pos + 1] != '\0') {
            try {
                port = boost::lexical_cast<boost::uint16_t>(server.substr(pos + 1));
            }
            catch(const boost::bad_lexical_cast&) {
                log()->error("Cannot parse memcached server port: %s", server.c_str());
                continue;
            }
        }
        
        setup.servers_.insert(std::make_pair(server.substr(0, pos), port));
    }
    
    std::string no_cache =
        config->as<std::string>("/xscript/tagged-cache-memcached/no-cache", StringUtils::EMPTY_STRING);

    insert2Cache(no_cache);
    
    pool_ = std::auto_ptr<MemcachedPool>(new MemcachedPool(workers, setup));
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
DocCacheMemcached::saveDoc(const TagKey *key, CacheContext *cache_ctx,
    const Tag &tag, const boost::shared_ptr<CacheData> &cache_data) {
    (void)cache_ctx;
    log()->debug("saving doc in memcached");
    
    std::string mc_key = key->asString();
    std::string val;

    // Adding Tag
    boost::int32_t time = std::min(tag.last_modified, HttpDateUtils::MAX_LIVE_TIME);
    val.append((char*)&time, sizeof(time));
    time = std::min(tag.expire_time, HttpDateUtils::MAX_LIVE_TIME);
    val.append((char*)&time, sizeof(time));

    std::string buf;
    cache_data->serialize(buf);
    val.append(buf);
    
    boost::uint32_t size = val.length();
    if (size > max_size_) {
        log()->info("object size %d exceeds limit %d", size, max_size_);
        return false;
    }
    
    memcached_return rv;
    {
        std::auto_ptr<MemcachedConnection> mc = pool_->get();
        rv = memcached_set(
            mc->get(), mc_key.c_str(), mc_key.length(), val.c_str(), val.length(), tag.expire_time, 0);
    }
    
    if (MEMCACHED_SUCCESS != rv) {
        log()->warn("Saving data to memcached failed: %d", rv);
        return false;
    }
    
    return true;
}

bool
DocCacheMemcached::loadDoc(const TagKey *key, CacheContext *cache_ctx,
    Tag &tag, boost::shared_ptr<CacheData> &cache_data) {
    (void)cache_ctx;
    log()->debug("loading doc in memcached");
    
    std::string mc_key = key->asString();
    
    size_t vallen = 0;
    uint32_t flags = 0;
    memcached_return rv;
    CharHelper val;
    
    {
        std::auto_ptr<MemcachedConnection> mc = pool_->get();
        val = CharHelper(memcached_get(mc->get(), mc_key.c_str(), mc_key.length(), &vallen, &flags, &rv));
    }

    if (NULL == val.get() || MEMCACHED_SUCCESS != rv) {
        if (MEMCACHED_NOTFOUND != rv) {
            log()->warn("Loading data from memcached failed: %d", rv);    
        }
        return false;
    }

    if (vallen <= 2*sizeof(boost::int32_t)) {
        log()->warn("incorrect data length while memcached loading");
        return false;
    }
    
    log()->debug("Loaded! %s", val.get());
    try {
        log()->debug("Parsing");

        char* value = val.get();

        tag.last_modified = *((boost::int32_t*)(value));
        value += sizeof(boost::int32_t);
        tag.expire_time = *((boost::int32_t*)(value));
        value += sizeof(boost::int32_t);
        vallen -= 2*sizeof(boost::int32_t);
        
        if (!DocCacheBase::checkTag(NULL, tag, "loading doc from memcached")) {
            return false;
        }
        
        return cache_data->parse(value, vallen);
    }
    catch (const std::exception &e) {
        log()->error("error while parsing doc from memcached: %s", e.what());
        return false;
    }
}

static ComponentRegisterer<DocCacheMemcached> reg_;

};
