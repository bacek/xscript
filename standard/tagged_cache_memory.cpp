#include "settings.h"

#include <map>
#include <list>
#include <cassert>
#include <algorithm>

#include "xscript/tag.h"
#include "xscript/util.h"
#include "xscript/config.h"
#include "xscript/logger.h"
#include "xscript/param.h"
#include "xscript/tagged_block.h"
#include "xscript/tagged_cache.h"

#include <boost/crc.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/checked_delete.hpp>
#include <boost/current_function.hpp>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

class TaggedCacheMemory;

class TagKeyMemory : public TagKey
{
public:
	TagKeyMemory(const Context *ctx, const TaggedBlock *block);
	virtual const std::string& asString() const;

private:
	std::string value_;
};

class TaggedCacheMemoryPool : private boost::noncopyable
{
public:
	explicit TaggedCacheMemoryPool(const TaggedCacheMemory* parent = NULL);
	virtual ~TaggedCacheMemoryPool();

	bool loadDoc(const TagKey &key, Tag &tag, XmlDocHelper &doc);
	bool saveDoc(const TagKey &key, const Tag& tag, const XmlDocHelper &doc);

	void clear();

private:
	class DocData;
	typedef std::map<std::string, DocData> Key2Data;
	typedef std::list<Key2Data::iterator> LRUList;

	class DocData
	{
	public:
		DocData();
		explicit DocData(LRUList::iterator list_pos);

		void assign(const Tag& tag, const xmlDocPtr ptr);

		xmlDocPtr copyDoc() const;
			
		void clearDoc();

	public:
		Tag tag;
		xmlDocPtr ptr;
		LRUList::iterator pos;
		time_t stored_time;
		bool prefetch_marked;
	};

	void shrink();
	void removeExpiredDocuments();

private:
	const TaggedCacheMemory* parent_;
	boost::mutex mutex_;

	Key2Data key2data_;
	LRUList list_;
};

class TaggedCacheMemory : public TaggedCache
{
public:
	TaggedCacheMemory();
	virtual ~TaggedCacheMemory();

	virtual void init(const Config *config);

	virtual time_t minimalCacheTime() const;
	
	virtual bool loadDoc(const TagKey *key, Tag &tag, XmlDocHelper &doc);
	virtual bool saveDoc(const TagKey *key, const Tag& tag, const XmlDocHelper &doc);
	virtual std::auto_ptr<TagKey> createKey(const Context *ctx, const TaggedBlock *block) const;

	unsigned int maxSize() const;

private:
	TaggedCacheMemoryPool* pool(const TagKey *key) const;

	static const int DEFAULT_POOL_COUNT;
	static const int DEFAULT_POOL_SIZE;
	static const time_t DEFAULT_CACHE_TIME;

private:
	time_t min_time_;
	unsigned int max_size_;
	std::vector<TaggedCacheMemoryPool*> pools_;
};

const int TaggedCacheMemory::DEFAULT_POOL_COUNT = 16;
const int TaggedCacheMemory::DEFAULT_POOL_SIZE = 128;
const time_t TaggedCacheMemory::DEFAULT_CACHE_TIME = 5; // sec

TagKeyMemory::TagKeyMemory(const Context *ctx, const TaggedBlock *block) : value_()
{
	assert(NULL != ctx);
	assert(NULL != block);

	if (!block->xsltName().empty()) {
		value_.assign(block->xsltName());
		value_.push_back('|');
	}
	value_.append(block->canonicalMethod(ctx));

	const std::vector<Param*> &v = block->params();
	for (std::vector<Param*>::const_iterator i = v.begin(), end = v.end(); i != end; ++i) {
		value_.append(":").append((*i)->asString(ctx));
	}
}

const std::string&
TagKeyMemory::asString() const {
	return value_;
}

TaggedCacheMemory::TaggedCacheMemory() : 
	min_time_(Tag::UNDEFINED_TIME), max_size_(0)
{
}

TaggedCacheMemory::~TaggedCacheMemory() {
	std::for_each(pools_.begin(), pools_.end(), boost::checked_deleter<TaggedCacheMemoryPool>());
}

std::auto_ptr<TagKey>
TaggedCacheMemory::createKey(const Context *ctx, const TaggedBlock *block) const {
	return std::auto_ptr<TagKey>(new TagKeyMemory(ctx, block));
}

void
TaggedCacheMemory::init(const Config *config) {

	assert(pools_.empty());

	unsigned int pools = config->as<unsigned int>("/xscript/tagged-cache-memory/pools", DEFAULT_POOL_COUNT);
	unsigned int pool_size = config->as<unsigned int>("/xscript/tagged-cache-memory/pool-size", DEFAULT_POOL_SIZE);

	max_size_ = pool_size;
	for (unsigned int i = 0; i < pools; ++i) {
		pools_.push_back(new TaggedCacheMemoryPool(this));
	}
	min_time_ = config->as<time_t>("/xscript/tagged-cache-memory/min-cache-time", DEFAULT_CACHE_TIME);
	if (min_time_ <= 0) {
		min_time_ = DEFAULT_CACHE_TIME;
	}
}

time_t
TaggedCacheMemory::minimalCacheTime() const {
	return min_time_;
}

bool
TaggedCacheMemory::loadDoc(const TagKey *key, Tag &tag, XmlDocHelper &doc) {
	TaggedCacheMemoryPool *mpool = pool(key);
	assert(NULL != mpool);
	return mpool->loadDoc(*key, tag, doc);
}

bool
TaggedCacheMemory::saveDoc(const TagKey *key, const Tag &tag, const XmlDocHelper &doc) {
	TaggedCacheMemoryPool *mpool = pool(key);
	assert(NULL != mpool);
	return mpool->saveDoc(*key, tag, doc);
}

unsigned int
TaggedCacheMemory::maxSize() const {
	return max_size_;
}

TaggedCacheMemoryPool*
TaggedCacheMemory::pool(const TagKey *key) const {
	assert(NULL != key);

	const unsigned int sz = pools_.size();
	assert(sz);

	const std::string &str = key->asString();

	boost::crc_32_type result;
	result.process_bytes(str.data(), str.size());
	unsigned int index = result.checksum() % sz;
	return pools_[index];
}


TaggedCacheMemoryPool::DocData::DocData() : tag(), ptr(NULL), pos(), prefetch_marked(false)
{
}

TaggedCacheMemoryPool::DocData::DocData(LRUList::iterator list_pos) :
	tag(), ptr(NULL), pos(list_pos), prefetch_marked(false)
{
}

void
TaggedCacheMemoryPool::DocData::assign(const Tag& t, const xmlDocPtr p) {
	assert(NULL != p);
	clearDoc();

	tag = t;
	ptr = xmlCopyDoc(p, 1);
	XmlUtils::throwUnless(NULL != ptr);
	stored_time = time(NULL);
	prefetch_marked = false;
}

xmlDocPtr
TaggedCacheMemoryPool::DocData::copyDoc() const {
	assert(ptr);
	xmlDocPtr p = xmlCopyDoc(ptr, 1);
	XmlUtils::throwUnless(NULL != p);
	return p;
}

void
TaggedCacheMemoryPool::DocData::clearDoc() {
	XmlDocHelper cleanup(ptr);
	ptr = NULL;
}


TaggedCacheMemoryPool::TaggedCacheMemoryPool(const TaggedCacheMemory* parent) :
	parent_(parent), mutex_(), key2data_(), list_()
{
}

TaggedCacheMemoryPool::~TaggedCacheMemoryPool() {
	clear();
}

bool
TaggedCacheMemoryPool::loadDoc(const TagKey &key, Tag &tag, XmlDocHelper &doc) {

	const std::string &str = key.asString();
	log()->debug("%s, key: %s", BOOST_CURRENT_FUNCTION, str.c_str());

	boost::mutex::scoped_lock lock(mutex_);

	if (list_.empty()) {
		return false;
	}
	
	Key2Data::iterator i = key2data_.find(str);
	if (i == key2data_.end()) {
		return false;
	}

	DocData &data = i->second;
	if (data.tag.expired()) {
		if (data.pos != list_.end()) {
			list_.erase(data.pos);
		}
		data.clearDoc();
		key2data_.erase(i);
		lock.unlock();
		log()->debug("%s, key: %s, expired", BOOST_CURRENT_FUNCTION, str.c_str());
		return false;
	}
	if (!data.prefetch_marked && data.tag.needPrefetch(data.stored_time)) {
		data.prefetch_marked = true;
		lock.unlock();
		log()->debug("%s, key: %s, prefetch", BOOST_CURRENT_FUNCTION, str.c_str());
		return false;
	}

	tag = data.tag;
	doc.reset(data.copyDoc());

	if (data.pos != list_.end()) {
		list_.erase(data.pos);
	}
	data.pos = list_.insert(list_.end(), i);

	lock.unlock();
	log()->info("%s, key: %s, loaded", BOOST_CURRENT_FUNCTION, str.c_str());
	return true;
}

bool
TaggedCacheMemoryPool::saveDoc(const TagKey &key, const Tag& tag, const XmlDocHelper &doc) {

	const std::string &str = key.asString();
	log()->debug("%s, key: %s", BOOST_CURRENT_FUNCTION, str.c_str());

	boost::mutex::scoped_lock lock(mutex_);

	Key2Data::iterator i = key2data_.find(str);
	if (i != key2data_.end()) {
		DocData &data = i->second;
		if (data.pos != list_.end()) {
			list_.erase(data.pos);
		}

		data.assign(tag, doc.get());
		data.pos = list_.insert(list_.end(), i);

		lock.unlock();
		log()->info("%s, key: %s, updated", BOOST_CURRENT_FUNCTION, str.c_str());
		return true;
	}

	shrink();

	i = key2data_.insert(std::make_pair(str, DocData(list_.end()))).first;
	DocData &data = i->second;
	data.assign(tag, doc.get());
	data.pos = list_.insert(list_.end(), i);

	lock.unlock();
	log()->info("%s, key: %s, inserted", BOOST_CURRENT_FUNCTION, str.c_str());
	return true;
}

void
TaggedCacheMemoryPool::clear() {
	boost::mutex::scoped_lock lock(mutex_);

	list_.clear();

	Key2Data tmp;
	tmp.swap(key2data_);

	for (Key2Data::iterator i = tmp.begin(), end = tmp.end(); i != end; ++i) {
		DocData &data = i->second;
		data.clearDoc();
	}
}

void
TaggedCacheMemoryPool::shrink() {
	if (list_.empty()) {
		return;
	}

	unsigned int max_num = parent_->maxSize();
	if (0 == max_num) {
		removeExpiredDocuments();
		return;
	}

	if (list_.size() < max_num) {
		return;
	}

	removeExpiredDocuments();

	// remove least recently used documents
	while (!list_.empty() && list_.size() >= max_num) {
		Key2Data::iterator i = *list_.begin();

		if (i != key2data_.end()) {
			DocData &data = i->second;
			log()->debug("%s, key: %s, shrink", BOOST_CURRENT_FUNCTION, i->first.c_str());
			data.clearDoc();
			key2data_.erase(i);
		}

		list_.pop_front();
	}
}

void
TaggedCacheMemoryPool::removeExpiredDocuments() {
	for (LRUList::iterator li = list_.begin(), end = list_.end(); li != end; ) {
		Key2Data::iterator i = *li;
		DocData &data = i->second;
		if (data.tag.expired()) {
			log()->debug("%s, key: %s, remove expired", BOOST_CURRENT_FUNCTION, i->first.c_str());
			data.clearDoc();
			key2data_.erase(i);
			list_.erase(li++);
		}
		else {
			++li;
		}
	}
}

static ComponentRegisterer<TaggedCache> reg_(new TaggedCacheMemory());

} // namespace xscript
