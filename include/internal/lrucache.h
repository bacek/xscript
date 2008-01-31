#ifndef _XSCRIPT_INTERNAL_LRUCACHE_H_
#define _XSCRIPT_INTERNAL_LRUCACHE_H_

#include "settings.h"
#include <stdexcept>

namespace xscript
{

template<typename Key, typename Data>
class LRUCache {
	struct ListElement;
	typedef std::list<ListElement> List;

#ifndef HAVE_HASHMAP
	typedef std::map<Key, typename List::iterator> Map;
#else
	typedef details::hash_map<Key, typename List::iterator> Map;
#endif

	class ListElement {
	public:
		ListElement(const Data& data, const typename Map::iterator& iterator) :
			data_(data), map_iterator_(iterator) { }

		Data data_;
		typename Map::iterator map_iterator_;
	};

	Map key2data_;
	List data_;
	unsigned int max_size_;

public:
	typedef typename Map::iterator iterator;
	typedef typename Map::const_iterator const_iterator;

	LRUCache(unsigned int size);
	~LRUCache();

	void clear();

	void erase(const Key& key);
	void erase(iterator it);

	iterator fetch(const Key& key);
	void insert(const Key& key, const Data& data);

	const_iterator begin() const;
	iterator begin();

	const_iterator end() const;
	iterator end();

	const Data& data(const_iterator it) const;
private:
	const_iterator find(const Key& key) const;
	iterator find(const Key& key);
	void update(iterator it, const Data& data);
};


template<typename Key, typename Data>
LRUCache<Key, Data>::LRUCache(unsigned int size) :
	max_size_(size)
{}

template<typename Key, typename Data>
LRUCache<Key, Data>::~LRUCache() {
}

template<typename Key, typename Data> void
LRUCache<Key, Data>::clear() {
	key2data_.clear();
	data_.clear();
}

template<typename Key, typename Data> void
LRUCache<Key, Data>::erase(LRUCache<Key, Data>::iterator it) {
	if (it != end()) {
		data_.erase(it->second);
		key2data_.erase(it);
		return;
	}
	throw std::out_of_range("invalid iterator in LRUCache");
}

template<typename Key, typename Data> void
LRUCache<Key, Data>::erase(const Key& key) {
	iterator it = find(key);
	erase(it);
}

template<typename Key, typename Data> typename LRUCache<Key, Data>::const_iterator
LRUCache<Key, Data>::find(const Key& key) const {
	return key2data_.find(key);
}

template<typename Key, typename Data> typename LRUCache<Key, Data>::iterator
LRUCache<Key, Data>::find(const Key& key) {
	return key2data_.find(key);
}

template<typename Key, typename Data> typename LRUCache<Key, Data>::iterator
LRUCache<Key, Data>::fetch(const Key& key) {
	iterator it = find(key);
	if (it != end()) {
		Data data = it->second->data_;
		update(it, data);
	}
	return it;
}

template<typename Key, typename Data> typename LRUCache<Key, Data>::iterator
LRUCache<Key, Data>::begin() {
	return key2data_.begin();
}

template<typename Key, typename Data> typename LRUCache<Key, Data>::const_iterator
LRUCache<Key, Data>::begin() const {
	return key2data_.begin();
}

template<typename Key, typename Data> typename LRUCache<Key, Data>::iterator
LRUCache<Key, Data>::end() {
	return key2data_.end();
}

template<typename Key, typename Data> typename LRUCache<Key, Data>::const_iterator
LRUCache<Key, Data>::end() const {
	return key2data_.end();
}

template<typename Key, typename Data> const Data&
LRUCache<Key, Data>::data(LRUCache<Key, Data>::const_iterator it) const {
	if (it != end()) {
		return it->second->data_;
	}
	
	throw std::out_of_range("invalid iterator in LRUCache");
}

template<typename Key, typename Data> void
LRUCache<Key, Data>::insert(const Key& key, const Data& data) {
	iterator it = find(key);
	if (it == end()) {
		if (data_.size() == max_size_) {
			key2data_.erase(data_.back().map_iterator_);
			data_.pop_back();
		}
		data_.push_front(ListElement(data, it));
		key2data_[key] = data_.begin();
		data_.begin()->map_iterator_ = find(key);
	}
	else {
		update(it, data);
	}
}

template<typename Key, typename Data> void
LRUCache<Key, Data>::update(LRUCache<Key, Data>::iterator it, const Data& data) {
	data_.erase(it->second);
	data_.push_front(ListElement(data, it));
	it->second = data_.begin();
}

} // namespace xscript

#endif // _XSCRIPT_INTERNAL_LRUCACHE_H_
