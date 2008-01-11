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
typedef std::list<XmlPtr> XmlList;

#ifndef HAVE_HASHMAP
typedef std::map<std::string, XmlList::iterator> XmlMap;
#else
typedef details::hash_map<std::string, XmlList::iterator, details::StringHash> XmlMap;
#endif

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
	
	void erase(const std::string &name);
	boost::shared_ptr<Xml> fetch(const std::string &name);
	void store(const std::string &name, const boost::shared_ptr<Xml> &xml);
	
private:
	void removeLast();
	bool expired(const boost::shared_ptr<Xml> &xml) const;
	void updateIterator(XmlMap::iterator iter, const boost::shared_ptr<Xml> &xml);
	
private:
	boost::mutex mutex_;
	
	bool enabled_;
	unsigned int max_size_;
	
	XmlMap iterators_;
	std::list<XmlPtr> documents_;
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

private:
	StandardScriptCache(const StandardScriptCache &);
	StandardScriptCache& operator = (const StandardScriptCache &);
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

private:
	StandardStylesheetCache(const StandardStylesheetCache &);
	StandardStylesheetCache& operator = (const StandardStylesheetCache &);
};

XmlStorage::XmlStorage(unsigned int max_size) :
	enabled_(true), max_size_(max_size)
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
	
	iterators_.clear();
	documents_.clear();
	
	enabled_ = false;
}

void
XmlStorage::enable() {
	log()->debug("enabling storage");
	boost::mutex::scoped_lock sl(mutex_);
	enabled_ = true;
}

void
XmlStorage::erase(const std::string &name) {
	
	log()->debug("erasing %s from storage", name.c_str());
	
	boost::mutex::scoped_lock sl(mutex_);
	if (!enabled_) {
		log()->debug("erasing from disabled storage");
		return;
	}
	
	XmlMap::iterator i = iterators_.find(name);
	if (iterators_.end() != i) {
		documents_.erase(i->second);
		iterators_.erase(i);
	}
}

boost::shared_ptr<Xml>
XmlStorage::fetch(const std::string &name) {
	
	log()->debug("trying to fetch %s from storage", name.c_str());
	
	boost::mutex::scoped_lock sl(mutex_);
	if (!enabled_) {
		log()->debug("fetching from disabled storage");
		return boost::shared_ptr<Xml>();
	}
	
	XmlMap::iterator i = iterators_.find(name);
	if (iterators_.end() == i) {
		return boost::shared_ptr<Xml>();
	}
	
	if (expired(*i->second)) {
		documents_.erase(i->second);
		iterators_.erase(i);
		return boost::shared_ptr<Xml>();
	}
	
	XmlPtr xml = (*i->second);
	updateIterator(i, xml);
	
	log()->debug("%s found in storage", name.c_str());
	return xml;
}

void
XmlStorage::store(const std::string &name, const boost::shared_ptr<Xml> &xml) {
	
	log()->debug("trying to store %s into storage", name.c_str());
	
	boost::mutex::scoped_lock sl(mutex_);
	if (!enabled_) {
		log()->debug("storing into disabled storage");
		return;
	}
	
	XmlMap::iterator i = iterators_.find(name);
	if (iterators_.end() == i) {
		removeLast();
		documents_.push_front(xml);
		iterators_[name] = documents_.begin();
	}
	else {
		updateIterator(i, xml);
	}
	log()->debug("storing of %s succeeded", name.c_str());
}

void
XmlStorage::removeLast() {

	if (documents_.size() == max_size_) {
		std::string name = documents_.back()->name();
		iterators_.erase(name);
		documents_.pop_back();
	}
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

void
XmlStorage::updateIterator(XmlMap::iterator i, const boost::shared_ptr<Xml> &xml) {
	documents_.erase(i->second);
	documents_.push_front(xml);
	i->second = documents_.begin();
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
	try {
		storages_.reserve(buckets);
		for (int i = 0; i < buckets; ++i) {
			storages_.push_back(new XmlStorage(bucksize));
		}
	}
	catch (const std::exception &e) {
		std::for_each(storages_.begin(), storages_.end(), boost::checked_deleter<XmlStorage>());
		storages_.clear();
		throw;
	}
	std::vector<std::string> names;
	config->subKeys(std::string("/xscript/").append(name).append("/deny"), names);
	for (std::vector<std::string>::iterator i = names.begin(), end = names.end(); i != end; ++i) {
		denied_.insert(config->as<std::string>(*i));
	}
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
