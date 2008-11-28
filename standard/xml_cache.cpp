#include "settings.h"

#include <list>
#include <vector>
#include <algorithm>

#include <boost/bind.hpp>
#include <boost/checked_delete.hpp>
#include <boost/crc.hpp>
#include <boost/cstdint.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/utility.hpp>

#include "xscript/cache_usage_counter.h"
#include "xscript/config.h"
#include "xscript/logger.h"
#include "xscript/operation_mode.h"
#include "xscript/policy.h"
#include "xscript/script.h"
#include "xscript/stylesheet.h"
#include "xscript/script_cache.h"
#include "xscript/stylesheet_cache.h"
#include "xscript/xml.h"

#include "internal/hash.h"
#include "internal/hashmap.h"
#include "internal/lrucache.h"

#ifndef HAVE_HASHMAP
#include <map>
#endif

#if defined(HAVE_STLPORT_HASHMAP)
#define HAVE_HASHSET 1
#include <hash_set>
#elif defined(HAVE_GNUCXX_HASHMAP) || defined(HAVE_EXT_HASH_MAP)
#define HAVE_HASHSET 1
#include <ext/hash_set>
#endif

#ifndef HAVE_HASHSET
#include <set>
#endif

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {
typedef boost::shared_ptr<Xml> XmlPtr;

#ifndef HAVE_HASHSET
typedef std::set<std::string> StringSet;
#elif defined(HAVE_GNUCXX_HASHMAP)
typedef __gnu_cxx::hash_set<std::string, details::StringHash> StringSet;
#elif defined(HAVE_STLPORT_HASHMAP) || defined(HAVE_EXT_HASH_MAP)
typedef std::hash_set<std::string, details::StringHash> StringSet;
#endif

class XmlStorage : private boost::noncopyable {
public:
    XmlStorage(unsigned int max_size, time_t refresh_delay);
    virtual ~XmlStorage();

    void clear();
    void enable();

    void erase(const std::string &key);
    boost::shared_ptr<Xml> fetch(const std::string &key);
    void store(const std::string &key, const boost::shared_ptr<Xml> &xml);

    const CacheUsageCounter* getCounter() const;

private:
    struct Element {
        Element(boost::shared_ptr<Xml> xml, time_t checked) : xml_(xml), checked_(checked)
        {}
        boost::shared_ptr<Xml> xml_;
        time_t checked_;
    };

    bool expired(const Element &element) const;

private:
    boost::mutex mutex_;
    bool enabled_;

    typedef LRUCache<std::string, Element> CacheType;
    CacheType cache_;
    time_t refresh_delay_;

    std::auto_ptr<CacheUsageCounter> counter_;
};

class XmlCache {
public:
    XmlCache();
    virtual ~XmlCache();

    virtual void clear();
    virtual void erase(const std::string &name);

    virtual void init(const char *name, const Config *config);

    virtual boost::shared_ptr<Xml> fetchXml(const std::string &name);
    virtual void storeXml(const std::string &name, const boost::shared_ptr<Xml> &xml);

protected:
    XmlStorage* findStorage(const std::string &name) const;
    void setStatBuilder(StatBuilder* statBuilder) {
        statBuilder_ = statBuilder;
    }

private:
    StringSet denied_;
    std::vector<XmlStorage*> storages_;
    StatBuilder* statBuilder_;

    class StorageContainerHolder {
    public:
        StorageContainerHolder();
        StorageContainerHolder(std::vector<XmlStorage*>& storages);
        ~StorageContainerHolder();
        void release();
    private:
        std::vector<XmlStorage*>* storages_;
    };
};

class StandardScriptCache : public XmlCache, public ScriptCache {
public:
    StandardScriptCache();
    virtual ~StandardScriptCache();

    virtual void init(const Config *config);

    virtual void clear();
    virtual void erase(const std::string &name);

    virtual boost::shared_ptr<Script> fetch(const std::string &name);
    virtual void store(const std::string &name, const boost::shared_ptr<Script> &script);

    virtual boost::mutex* getMutex(const std::string &name);

private:
    static const unsigned int NUMBER_OF_MUTEXES = 256;
    boost::mutex mutexes_[NUMBER_OF_MUTEXES];
};

class StandardStylesheetCache : public XmlCache, public StylesheetCache {
public:
    StandardStylesheetCache();
    virtual ~StandardStylesheetCache();

    virtual void init(const Config *config);

    virtual void clear();
    virtual void erase(const std::string &name);

    virtual boost::shared_ptr<Stylesheet> fetch(const std::string &name);
    virtual void store(const std::string &name, const boost::shared_ptr<Stylesheet> &stylesheet);

    virtual boost::mutex* getMutex(const std::string &name);

private:
    static const unsigned int NUMBER_OF_MUTEXES = 256;
    boost::mutex mutexes_[NUMBER_OF_MUTEXES];
};

XmlStorage::XmlStorage(unsigned int max_size, time_t refresh_delay) :
        enabled_(true), cache_(max_size), refresh_delay_(refresh_delay),
        counter_(CacheUsageCounterFactory::instance()->createCounter("xml-storage")) {
    counter_->max(max_size);
}

XmlStorage::~XmlStorage() {
    boost::mutex::scoped_lock sl(mutex_);
}

void
XmlStorage::clear() {

    log()->debug("disabling storage");
    boost::mutex::scoped_lock sl(mutex_);
    if (!enabled_) {
        return;
    }
    cache_.clear();
    enabled_ = false;

    counter_->reset();
}

void
XmlStorage::enable() {
    log()->debug("enabling storage");
    boost::mutex::scoped_lock sl(mutex_);
    enabled_ = true;
}

void
XmlStorage::erase(const std::string &key) {

    log()->debug("erasing %s from storage", key.c_str());

    boost::mutex::scoped_lock sl(mutex_);
    if (!enabled_) {
        log()->debug("erasing from disabled storage");
        return;
    }

    cache_.erase(key);
    counter_->removed(key);
}

boost::shared_ptr<Xml>
XmlStorage::fetch(const std::string &key) {

    log()->debug("trying to fetch %s from storage", key.c_str());

    boost::mutex::scoped_lock sl(mutex_);
    if (!enabled_) {
        log()->debug("fetching from disabled storage");
        return boost::shared_ptr<Xml>();
    }

    CacheType::iterator it = cache_.fetch(key);
    if (cache_.end() == it) {
        return boost::shared_ptr<Xml>();
    }

    if (expired(cache_.data(it))) {
        cache_.erase(it);
        counter_->removed(key);
        return boost::shared_ptr<Xml>();
    }

    log()->debug("%s found in storage", key.c_str());
    counter_->fetched(key);

    return cache_.data(it).xml_;
}

void
XmlStorage::store(const std::string &key, const boost::shared_ptr<Xml> &xml) {

    log()->debug("trying to store %s into storage", key.c_str());

    boost::mutex::scoped_lock sl(mutex_);
    if (!enabled_) {
        log()->debug("storing into disabled storage");
        return;
    }

    cache_.insert(key, Element(xml, time(NULL)), counter_.get());
    counter_->stored(key);
    log()->debug("storing of %s succeeded", key.c_str());
}

bool
XmlStorage::expired(const Element &element) const {

    log()->debug("checking whether xml expired");

    if (element.checked_ > time(NULL) - refresh_delay_) {
        return false;
    }

    const Xml::TimeMapType& modified_info = element.xml_->modifiedInfo();
    for(Xml::TimeMapType::const_iterator it = modified_info.begin(), end = modified_info.end();
        it != end;
        ++it) {
        std::time_t modified;
        try {
            modified = FileUtils::modified(it->first);
        }
        catch(const boost::filesystem::filesystem_error &e) {
            return true;
        }

        log()->debug("is included xml %s expired: %llu, %llu", it->first.c_str(),
            static_cast<unsigned long long>(modified),
            static_cast<unsigned long long>(it->second));

        if (modified != it->second) {
            return true;
        }
    }

    return false;
}

const CacheUsageCounter*
XmlStorage::getCounter() const {
    return counter_.get();
}

XmlCache::XmlCache() : statBuilder_(NULL) {
}

XmlCache::~XmlCache() {
    std::for_each(storages_.begin(), storages_.end(), boost::checked_deleter<XmlStorage>());
}

void
XmlCache::clear() {
    std::for_each(storages_.begin(), storages_.end(), boost::bind(&XmlStorage::clear, _1));
    std::for_each(storages_.begin(), storages_.end(), boost::bind(&XmlStorage::enable, _1));
}

void
XmlCache::erase(const std::string &name) {
    StringSet::const_iterator i = denied_.find(name);
    if (denied_.end() == i) {
        std::string cache_name = Policy::instance()->getKey(NULL, name);
        findStorage(name)->erase(cache_name);
    }
}

void
XmlCache::init(const char *name, const Config *config) {

    int buckets = config->as<int>(std::string("/xscript/").append(name).append("/buckets"), 10);
    int bucksize = config->as<int>(std::string("/xscript/").append(name).append("/bucket-size"), 200);

    size_t delay = 0;
    if (OperationMode::instance()->isProduction()) {
        delay = config->as<size_t>(std::string("/xscript/").append(name).append("/refresh-delay"), 5);
    }

    StorageContainerHolder holder(storages_);
    storages_.reserve(buckets);
    for (int i = 0; i < buckets; ++i) {
        storages_.push_back(new XmlStorage(bucksize, delay));
    }
    std::vector<std::string> names;
    config->subKeys(std::string("/xscript/").append(name).append("/deny"), names);
    for (std::vector<std::string>::iterator i = names.begin(), end = names.end(); i != end; ++i) {
        denied_.insert(config->as<std::string>(*i));
    }
    holder.release();

    for (std::vector<XmlStorage*>::iterator it = storages_.begin();
        it != storages_.end();
        ++it) {
        statBuilder_->addCounter((*it)->getCounter());
    }
}

boost::shared_ptr<Xml>
XmlCache::fetchXml(const std::string &name) {

    StringSet::const_iterator i = denied_.find(name);
    if (denied_.end() != i) {
        return boost::shared_ptr<Xml>();
    }
    std::string cache_name = Policy::instance()->getKey(NULL, name);
    return findStorage(name)->fetch(cache_name);
}

void
XmlCache::storeXml(const std::string &name, const boost::shared_ptr<Xml> &xml) {

    assert(NULL != xml.get());
    StringSet::const_iterator i = denied_.find(name);
    if (denied_.end() == i) {
        std::string cache_name = Policy::instance()->getKey(NULL, name);
        findStorage(name)->store(cache_name, xml);
    }
}

XmlStorage*
XmlCache::findStorage(const std::string &name) const {
    return storages_[HashUtils::crc32(name) % storages_.size()];
}


XmlCache::StorageContainerHolder::StorageContainerHolder():
        storages_(NULL) {}

XmlCache::StorageContainerHolder::StorageContainerHolder(std::vector<XmlStorage*>& storages):
        storages_(&storages) {}

XmlCache::StorageContainerHolder::~StorageContainerHolder() {
    if (NULL != storages_) {
        std::for_each(storages_->begin(), storages_->end(), boost::checked_deleter<XmlStorage>());
        storages_->clear();
    }
}

void
XmlCache::StorageContainerHolder::release() {
    storages_ = NULL;
}

StandardScriptCache::StandardScriptCache() {
}

StandardScriptCache::~StandardScriptCache() {
}

void
StandardScriptCache::init(const Config *config) {
    XmlCache::setStatBuilder(&getStatBuilder());
    XmlCache::init("script-cache", config);
}

void
StandardScriptCache::clear() {
    XmlCache::clear();
}

void
StandardScriptCache::erase(const std::string &name) {
    XmlCache::erase(name);
}

boost::shared_ptr<Script>
StandardScriptCache::fetch(const std::string &name) {
    return boost::dynamic_pointer_cast<Script>(fetchXml(name));
}

void
StandardScriptCache::store(const std::string &name, const boost::shared_ptr<Script> &script) {
    assert(NULL != script.get());
    storeXml(name, boost::dynamic_pointer_cast<Xml>(script));
}

boost::mutex*
StandardScriptCache::getMutex(const std::string &name) {
    return mutexes_ + HashUtils::crc32(name) % NUMBER_OF_MUTEXES;
}

StandardStylesheetCache::StandardStylesheetCache() {
}

StandardStylesheetCache::~StandardStylesheetCache() {
}

void
StandardStylesheetCache::init(const Config *config) {
    XmlCache::setStatBuilder(&getStatBuilder());
    XmlCache::init("stylesheet-cache", config);
}

void
StandardStylesheetCache::clear() {
    XmlCache::clear();
}

void
StandardStylesheetCache::erase(const std::string &name) {
    XmlCache::erase(name);
}

boost::shared_ptr<Stylesheet>
StandardStylesheetCache::fetch(const std::string &name) {
    return boost::dynamic_pointer_cast<Stylesheet>(fetchXml(name));
}

void
StandardStylesheetCache::store(const std::string &name, const boost::shared_ptr<Stylesheet> &stylesheet) {
    assert(NULL != stylesheet.get());
    storeXml(name, boost::dynamic_pointer_cast<Xml>(stylesheet));
}

boost::mutex*
StandardStylesheetCache::getMutex(const std::string &name) {
    return mutexes_ + HashUtils::crc32(name) % NUMBER_OF_MUTEXES;
}

static ComponentRegisterer<ScriptCache> script_reg_(new StandardScriptCache());
static ComponentRegisterer<StylesheetCache> stylesheet_reg_(new StandardStylesheetCache());

} // namespace xscript
