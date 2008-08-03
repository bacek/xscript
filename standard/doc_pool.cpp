#include <boost/current_function.hpp>
#include "xscript/logger.h"
#include "xscript/util.h"
#include "doc_pool.h"

namespace xscript 
{

DocPool::DocPool(size_t capacity, const std::string& name) :
	capacity_(capacity), 
    counter_(name), 
    memoryCounter_(AverageCounterFactory::instance()->createCounter(name+"-memory"))
{
}

DocPool::~DocPool() {
	clear();
}

const CacheCounter* DocPool::getCounter() const {
	return &counter_;
}

const AverageCounter* DocPool::getMemoryCounter() const {
	return memoryCounter_.get();
}

bool
DocPool::loadDoc(const TagKey &key, Tag &tag, XmlDocHelper &doc) {
	std::string keyStr = key.asString();
	DocPool::LoadResult res = loadDocImpl(keyStr, tag, doc);

	switch (res) {
		case DocPool::LOAD_NOT_FOUND:
			log()->info("%s: key: %s, not found", BOOST_CURRENT_FUNCTION, keyStr.c_str());
			break;
		case DocPool::LOAD_EXPIRED:
			log()->info("%s: key: %s, expired", BOOST_CURRENT_FUNCTION, keyStr.c_str());
			break;
		case DocPool::LOAD_NEED_PREFETCH:
			log()->info("%s: key: %s, need prefetch", BOOST_CURRENT_FUNCTION, keyStr.c_str());
			break;
		default:
			break;
	}

	return res == DocPool::LOAD_SUCCESSFUL;
}

DocPool::LoadResult
DocPool::loadDocImpl(const std::string &keyStr, Tag &tag, XmlDocHelper &doc) {
	log()->debug("%s, key: %s", BOOST_CURRENT_FUNCTION, keyStr.c_str());

	boost::mutex::scoped_lock lock(mutex_);

	if (list_.empty()) {
		return LOAD_NOT_FOUND;
	}
	
	Key2Data::iterator i = key2data_.find(keyStr);
	if (i == key2data_.end()) {
		return LOAD_NOT_FOUND;
	}

	DocData &data = i->second;
	if (data.tag.expired()) {
		if (data.pos != list_.end()) {
			list_.erase(data.pos);
		}
		counter_.decUsedMemory(data.doc_size);
		counter_.incRemoved();
		data.clearDoc();
		key2data_.erase(i);
		return LOAD_EXPIRED;
	}
	if (!data.prefetch_marked && data.tag.needPrefetch(data.stored_time)) {
		data.prefetch_marked = true;
		return LOAD_NEED_PREFETCH;
	}

	tag = data.tag;
	doc.reset(data.copyDoc());

	if (data.pos != list_.end()) {
		list_.erase(data.pos);
	}
	data.pos = list_.insert(list_.end(), i);

	counter_.incLoaded();
	return LOAD_SUCCESSFUL;
}

bool
DocPool::saveDoc(const TagKey &key, const Tag& tag, const XmlDocHelper &doc) {
	std::string keyStr = key.asString();
	DocPool::SaveResult res = saveDocImpl(keyStr, tag, doc);
	switch (res) {
		case DocPool::SAVE_STORED:
			log()->info("%s: key: %s, stored", BOOST_CURRENT_FUNCTION, keyStr.c_str());
			break;
		case DocPool::SAVE_UPDATED:
			log()->info("%s: key: %s, updated", BOOST_CURRENT_FUNCTION, keyStr.c_str());
			break;
	}

	return true;
}

DocPool::SaveResult
DocPool::saveDocImpl(const std::string &keyStr, const Tag& tag, const XmlDocHelper &doc) {

	log()->debug("%s, key: %s", BOOST_CURRENT_FUNCTION, keyStr.c_str());

	boost::mutex::scoped_lock lock(mutex_);

	counter_.incStored();

	SaveResult res = SAVE_STORED;
	Key2Data::iterator i = key2data_.find(keyStr);
	if (i == key2data_.end()) {
		shrink();
		i = key2data_.insert(std::make_pair(keyStr, DocData(list_.end()))).first;
		res = SAVE_UPDATED;
	}

	saveAtIterator(i, tag, doc);
	return res;
}


void DocPool::saveAtIterator(const Key2Data::iterator& i, const Tag& tag, const XmlDocHelper& doc) {
	DocData &data = i->second;
		
	if (data.pos != list_.end()) {
		list_.erase(data.pos);
	}

	counter_.decUsedMemory(data.doc_size);
	memoryCounter_->remove(data.doc_size);

	data.assign(tag, doc.get());

	counter_.incUsedMemory(data.doc_size);
	memoryCounter_->add(data.doc_size);
	data.pos = list_.insert(list_.end(), i);
}

void
DocPool::clear() {
	boost::mutex::scoped_lock lock(mutex_);

	list_.clear();

	Key2Data tmp;
	tmp.swap(key2data_);

	for (Key2Data::iterator i = tmp.begin(), end = tmp.end(); i != end; ++i) {
		DocData &data = i->second;
		counter_.decUsedMemory(data.doc_size);
		data.clearDoc();
	}
}

void
DocPool::shrink() {
	if (list_.empty()) {
		return;
	}

	if (0 == capacity_) {
		removeExpiredDocuments();
		return;
	}

	if (list_.size() < capacity_) {
		return;
	}

	removeExpiredDocuments();

	// remove least recently used documents
	while (!list_.empty() && list_.size() >= capacity_) {
		Key2Data::iterator i = *list_.begin();

		if (i != key2data_.end()) {
			DocData &data = i->second;
			log()->debug("%s, key: %s, shrink", BOOST_CURRENT_FUNCTION, i->first.c_str());
			counter_.decUsedMemory(data.doc_size);
			counter_.incRemoved();
			data.clearDoc();
			key2data_.erase(i);
		}

		list_.pop_front();
	}
}

void
DocPool::removeExpiredDocuments() {
	for (LRUList::iterator li = list_.begin(), end = list_.end(); li != end; ) {
		Key2Data::iterator i = *li;
		DocData &data = i->second;
		if (data.tag.expired()) {
			log()->debug("%s, key: %s, remove expired", BOOST_CURRENT_FUNCTION, i->first.c_str());
			counter_.decUsedMemory(data.doc_size);
			counter_.incRemoved();
			data.clearDoc();
			key2data_.erase(i);
			list_.erase(li++);
		}
		else {
			++li;
		}
	}
}



DocPool::DocData::DocData() : tag(), ptr(NULL), pos(), prefetch_marked(false), doc_size(0)
{
}

DocPool::DocData::DocData(LRUList::iterator list_pos) :
	tag(), ptr(NULL), pos(list_pos), prefetch_marked(false), doc_size(0)
{
}

void
DocPool::DocData::assign(const Tag& t, const xmlDocPtr p) {
	assert(NULL != p);
	clearDoc();

	tag = t;
	size_t s1 = getAllocatedMemory();
	ptr = xmlCopyDoc(p, 1);
	size_t s2 = getAllocatedMemory();

	doc_size = s2-s1;

	XmlUtils::throwUnless(NULL != ptr);
	stored_time = time(NULL);
	prefetch_marked = false;
}

xmlDocPtr
DocPool::DocData::copyDoc() const {
	assert(ptr);
	xmlDocPtr p = xmlCopyDoc(ptr, 1);
	XmlUtils::throwUnless(NULL != p);
	return p;
}

void
DocPool::DocData::clearDoc() {
	XmlDocHelper cleanup(ptr);
	ptr = NULL;
}

}
