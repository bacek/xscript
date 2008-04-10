#include "settings.h"

#include <list>
#include <vector>
#include <algorithm>
#include <boost/crc.hpp>
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>
#include <boost/utility.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/checked_delete.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include "xscript/xml.h"
#include "xscript/logger.h"
#include "xscript/config.h"
#include "xscript/vhost_data.h"
#include "xscript/script.h"
#include "xscript/stylesheet.h"
#include "xscript/script_cache.h"
#include "xscript/stylesheet_cache.h"

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

namespace xscript
{
typedef boost::shared_ptr<Xml> XmlPtr;

#ifndef HAVE_HASHSET
typedef std::set<std::string> StringSet;
#elif defined(HAVE_GNUCXX_HASHMAP)
typedef __gnu_cxx::hash_set<std::string, details::StringHash> StringSet;
#elif defined(HAVE_STLPORT_HASHMAP) || defined(HAVE_EXT_HASH_MAP)
typedef std::hash_set<std::string, details::StringHash> StringSet;
#endif

class XmlStorage : private boost::noncopyable
{
public:
	XmlStorage(unsigned int max_size);
	virtual ~XmlStorage();

	void clear();
	void enable();
	
	void erase(const std::string &key);
	boost::shared_ptr<Xml> fetch(const std::string &key);
	void store(const std::string &key, const boost::shared_ptr<Xml> &xml);
	
private:
	bool expired(const boost::shared_ptr<Xml> &xml) const;
	
private:
	boost::mutex mutex_;
	bool enabled_;

	typedef LRUCache<std::string, boost::shared_ptr<Xml> > CacheType;
	CacheType cache_;
};

class XmlCache
{
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
	
private:
	StringSet denied_;
	std::vector<XmlStorage*> storages_;

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

class StandardScriptCache : public XmlCache, public ScriptCache
{
public:
	StandardScriptCache();
	virtual ~StandardScriptCache();

	virtual void init(const Config *config);
	
	virtual void clear();
	virtual void erase(const std::string &name);
	
	virtual boost::shared_ptr<Script> fetch(const std::string &name);
	virtual void store(const std::string &name, const boost::shared_ptr<Script> &script);
};

class StandardStylesheetCache : public XmlCache, public StylesheetCache
{
public:
	StandardStylesheetCache();
	virtual ~StandardStylesheetCache();

	virtual void init(const Config *config);
	
	virtual void clear();
	virtual void erase(const std::string &name);
	
	virtual boost::shared_ptr<Stylesheet> fetch(const std::string &name);
	virtual void store(const std::string &name, const boost::shared_ptr<Stylesheet> &stylesheet);
};

XmlStorage::XmlStorage(unsigned int max_size) :
	enabled_(true), cache_(max_size)
{
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
		return boost::shared_ptr<Xml>();
	}

	log()->debug("%s found in storage", key.c_str());
	return cache_.data(it);
}

void
XmlStorage::store(const std::string &key, const boost::shared_ptr<Xml> &xml) {
	
	log()->debug("trying to store %s into storage", key.c_str());
	
	boost::mutex::scoped_lock sl(mutex_);
	if (!enabled_) {
		log()->debug("storing into disabled storage");
		return;
	}

	cache_.insert(key, xml);
	log()->debug("storing of %s succeeded", key.c_str());
}

bool
XmlStorage::expired(const boost::shared_ptr<Xml> &xml) const {

	namespace fs = boost::filesystem;
	log()->debug("checking whether xml expired");
	
	fs::path path(xml->name());
	std::time_t modified = fs::last_write_time(path);
	
	log()->debug("is xml expired: %llu, %llu", static_cast<unsigned long long>(modified),
		static_cast<unsigned long long>(xml->modified()));
	return modified > xml->modified();
}

XmlCache::XmlCache() {
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
		std::string cache_name = VirtualHostData::instance()->getKey(NULL, name);
		findStorage(name)->erase(cache_name);
	}
}

void
XmlCache::init(const char *name, const Config *config) {
	
	int buckets = config->as<int>(std::string("/xscript/").append(name).append("/buckets"), 10);
	int bucksize = config->as<int>(std::string("/xscript/").append(name).append("/bucket-size"), 200);

	StorageContainerHolder holder(storages_);
	storages_.reserve(buckets);
	for (int i = 0; i < buckets; ++i) {
		storages_.push_back(new XmlStorage(bucksize));
	}
	std::vector<std::string> names;
	config->subKeys(std::string("/xscript/").append(name).append("/deny"), names);
	for (std::vector<std::string>::iterator i = names.begin(), end = names.end(); i != end; ++i) {
		denied_.insert(config->as<std::string>(*i));
	}
	holder.release();
}

boost::shared_ptr<Xml>
XmlCache::fetchXml(const std::string &name) {

	StringSet::const_iterator i = denied_.find(name);
	if (denied_.end() != i) {
		return boost::shared_ptr<Xml>();
	}
	std::string cache_name = VirtualHostData::instance()->getKey(NULL, name);
	return findStorage(name)->fetch(cache_name);
}

void
XmlCache::storeXml(const std::string &name, const boost::shared_ptr<Xml> &xml) {
	
	assert(NULL != xml.get());
	StringSet::const_iterator i = denied_.find(name);
	if (denied_.end() == i) {
		std::string cache_name = VirtualHostData::instance()->getKey(NULL, name);
		findStorage(name)->store(cache_name, xml);
	}
}

XmlStorage*
XmlCache::findStorage(const std::string &name) const {
	boost::crc_32_type crc;
	crc.process_bytes(name.data(), name.size());
	return storages_[crc.checksum() % storages_.size()];
}


XmlCache::StorageContainerHolder::StorageContainerHolder():
	storages_(NULL)
{}

XmlCache::StorageContainerHolder::StorageContainerHolder(std::vector<XmlStorage*>& storages):
	storages_(&storages)
{}

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

StandardScriptCache::StandardScriptCache()
{
}

StandardScriptCache::~StandardScriptCache() {
}

void
StandardScriptCache::init(const Config *config) {
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

StandardStylesheetCache::StandardStylesheetCache()
{
}

StandardStylesheetCache::~StandardStylesheetCache() {
}

void
StandardStylesheetCache::init(const Config *config) {
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

static ComponentRegisterer<ScriptCache> script_reg_(new StandardScriptCache());
static ComponentRegisterer<StylesheetCache> stylesheet_reg_(new StandardStylesheetCache());

} // namespace xscript
