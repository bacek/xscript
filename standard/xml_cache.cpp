#include "settings.h"

#include <algorithm>
#include <list>
#include <vector>
#include <stdexcept>

#include <boost/bind.hpp>
#include <boost/checked_delete.hpp>
#include <boost/crc.hpp>
#include <boost/cstdint.hpp>
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
#include "xscript/util.h"
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

class XmlStorage;

class XmlCache {
public:
    XmlCache();
    virtual ~XmlCache();

    virtual void clear();

    virtual void init(const Config *config, StatBuilder &statBuilder);

    virtual boost::shared_ptr<Xml> fetchXml(const std::string &name);
    virtual void storeXml(const std::string &name, const boost::shared_ptr<Xml> &xml);

    static time_t delay();
    static time_t max_delay();

protected:
    XmlStorage* findStorage(const std::string &name) const;

private:
    StringSet denied_;
    std::vector<XmlStorage*> storages_;
    static time_t delay_;
    static time_t max_delay_;

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

class XmlStorage : private boost::noncopyable {
public:
    XmlStorage(unsigned int size);
    virtual ~XmlStorage();

    void clear();

    boost::shared_ptr<Xml> fetch(const std::string &key);
    void store(const std::string &key, const boost::shared_ptr<Xml> &xml);

    const CacheCounter* getCounter() const;

private:
    struct Element {
        Element() : check_time_(0), modified_(false) {}
        Element(boost::shared_ptr<Xml> xml) : xml_(xml), check_time_(time(NULL) + XmlCache::delay()), modified_(false) {}

        boost::shared_ptr<Xml> xml_;
        time_t check_time_;
        bool modified_;

        bool checkModified() const;
    };

    struct XmlExpiredFunc {
        bool operator() (Element &element, const Tag &tag) const;
    };
    
    typedef LRUCache<std::string, Element, XmlExpiredFunc> CacheType;
    std::auto_ptr<CacheCounter> counter_;
    std::auto_ptr<CacheType> cache_;
};

class StandardScriptCache : public XmlCache, public ScriptCache {
public:
    StandardScriptCache();
    virtual ~StandardScriptCache();

    virtual void init(const Config *config);

    virtual void clear();

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

    virtual boost::shared_ptr<Stylesheet> fetch(const std::string &name);
    virtual void store(const std::string &name, const boost::shared_ptr<Stylesheet> &stylesheet);

    virtual boost::mutex* getMutex(const std::string &name);

private:
    static const unsigned int NUMBER_OF_MUTEXES = 256;
    boost::mutex mutexes_[NUMBER_OF_MUTEXES];
};

XmlStorage::XmlStorage(unsigned int size) :
    counter_(CacheCounterFactory::instance()->createCounter("xml-storage"))
{
    cache_ = std::auto_ptr<CacheType>(new CacheType(size, false, *counter_));
}

XmlStorage::~XmlStorage() {
}

void
XmlStorage::clear() {
    log()->debug("disabling storage");
    cache_->clear();
}

boost::shared_ptr<Xml>
XmlStorage::fetch(const std::string &key) {
    log()->debug("trying to fetch %s from storage", key.c_str());
    Element element;
    Tag tag;
    if (!cache_->load(key, element, tag)) {
        return boost::shared_ptr<Xml>(); 
    }    
    log()->debug("%s found in storage", key.c_str());
    return element.xml_;
}

void
XmlStorage::store(const std::string &key, const boost::shared_ptr<Xml> &xml) {
    log()->debug("trying to store %s into storage", key.c_str());
    Tag tag;
    Element element(xml);
    cache_->save(key, element, tag);
    log()->debug("storing of %s succeeded", key.c_str());
}

const CacheCounter*
XmlStorage::getCounter() const {
    return counter_.get();
}

bool
XmlStorage::XmlExpiredFunc::operator() (Element &element, const Tag &tag) const {
    (void)tag;
    log()->debug("checking whether xml expired");

    if (!element.modified_) {
        time_t now = time(NULL);
        if (now >= element.check_time_ || (now + XmlCache::max_delay() < element.check_time_)) {
            if (element.checkModified()) {
                element.modified_ = true;
            }
            element.check_time_ = now + XmlCache::delay();
        }
    }
    return element.modified_;
}

bool XmlStorage::Element::checkModified() const {
    const Xml::TimeMapType& modified_info = xml_->modifiedInfo();
    for(Xml::TimeMapType::const_iterator it = modified_info.begin(), end = modified_info.end();
        it != end;
        ++it) {
        std::time_t modified;
        try {
            modified = FileUtils::modified(it->first);
        }
        catch(const std::exception &e) {
            modified = 0;
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

time_t XmlCache::delay_ = 0;
time_t XmlCache::max_delay_ = 0;

XmlCache::XmlCache() {
}

XmlCache::~XmlCache() {
    std::for_each(storages_.begin(), storages_.end(), boost::checked_deleter<XmlStorage>());
}

void
XmlCache::clear() {
    std::for_each(storages_.begin(), storages_.end(), boost::bind(&XmlStorage::clear, _1));
}

void
XmlCache::init(const Config *config, StatBuilder &statBuilder) {

    const std::string& name = statBuilder.getName();
    const std::string section = "/xscript/" + name;
    config->addForbiddenKey(section + "/*");
    
    int buckets = config->as<int>(section + "/buckets", 20);
    int bucksize = config->as<int>(section + "/bucket-size", 30);

    if (OperationMode::instance()->isProduction()) {
        delay_ = config->as<time_t>(section + "/refresh-delay", 5);
        max_delay_ = delay_ * 12; // 60
    }

    StorageContainerHolder holder(storages_);
    storages_.reserve(buckets);
    for (int i = 0; i < buckets; ++i) {
        storages_.push_back(new XmlStorage(bucksize));
    }
    holder.release();

    for (std::vector<XmlStorage*>::iterator it = storages_.begin();
        it != storages_.end();
        ++it) {
        statBuilder.addCounter((*it)->getCounter());
    }
}

time_t
XmlCache::delay() {
    return delay_;
}

time_t
XmlCache::max_delay() {
    return max_delay_;
}

boost::shared_ptr<Xml>
XmlCache::fetchXml(const std::string &name) {
    return findStorage(name)->fetch(Policy::instance()->getKey(NULL, name));
}

void
XmlCache::storeXml(const std::string &name, const boost::shared_ptr<Xml> &xml) {
    assert(NULL != xml.get());
    findStorage(name)->store(Policy::instance()->getKey(NULL, name), xml);
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
    XmlCache::init(config, statBuilder());
}

void
StandardScriptCache::clear() {
    XmlCache::clear();
}

boost::shared_ptr<Script>
StandardScriptCache::fetch(const std::string &name) {
    boost::shared_ptr<Script> script =
        boost::dynamic_pointer_cast<Script>(fetchXml(name));
    return NULL == script.get() || script->valid() ? script : boost::shared_ptr<Script>();
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
    XmlCache::init(config, statBuilder());
}

void
StandardStylesheetCache::clear() {
    XmlCache::clear();
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

static ComponentImplRegisterer<ScriptCache> script_reg_(new StandardScriptCache());
static ComponentImplRegisterer<StylesheetCache> stylesheet_reg_(new StandardStylesheetCache());

} // namespace xscript
