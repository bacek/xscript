#ifndef _XSCRIPT_INTERNAL_LRUCACHE_H_
#define _XSCRIPT_INTERNAL_LRUCACHE_H_

#include <stdexcept>
#include "xscript/cache_usage_counter.h"

namespace xscript {

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

    explicit LRUCache(unsigned int size);
    ~LRUCache();

    void clear();

    void erase(const Key& key);
    void erase(iterator it);

    iterator fetch(const Key& key);
    void insert(const Key& key, const Data& data, CacheUsageCounter* counter);

    const_iterator begin() const;
    const_iterator end() const;

    const Data& data(const_iterator it) const;
};


template<typename Key, typename Data>
LRUCache<Key, Data>::LRUCache(unsigned int size) :
        max_size_(size) {
}

template<typename Key, typename Data>
LRUCache<Key, Data>::~LRUCache() {
}

template<typename Key, typename Data> void
LRUCache<Key, Data>::clear() {
    key2data_.clear();
    data_.clear();
}

template<typename Key, typename Data> void
LRUCache<Key, Data>::erase(iterator it) {
    if (it == key2data_.end()) {
        throw std::out_of_range("invalid iterator in LRUCache");
    }
    data_.erase(it->second);
    key2data_.erase(it);
}

template<typename Key, typename Data> void
LRUCache<Key, Data>::erase(const Key& key) {
    iterator it = key2data_.find(key);
    erase(it);
}

template<typename Key, typename Data> typename LRUCache<Key, Data>::iterator
LRUCache<Key, Data>::fetch(const Key& key) {
    iterator map_it = key2data_.find(key);
    if (map_it != key2data_.end()) {
        typename List::iterator list_it = map_it->second;
        if (list_it != data_.begin() && list_it != data_.end()) {
            data_.splice(data_.begin(), data_, list_it);
            map_it->second = data_.begin();
	}
    }
    return map_it;
}

template<typename Key, typename Data> typename LRUCache<Key, Data>::const_iterator
LRUCache<Key, Data>::begin() const {
    return key2data_.begin();
}

template<typename Key, typename Data> typename LRUCache<Key, Data>::const_iterator
LRUCache<Key, Data>::end() const {
    return key2data_.end();
}

template<typename Key, typename Data> const Data&
LRUCache<Key, Data>::data(const_iterator it) const {
    if (it == key2data_.end()) {
        throw std::out_of_range("invalid iterator in LRUCache");
    }
    return it->second->data_;
}

template<typename Key, typename Data> void
LRUCache<Key, Data>::insert(const Key& key, const Data& data, CacheUsageCounter* counter) {
    iterator it = key2data_.find(key);
    if (it == key2data_.end()) {
        if (key2data_.size() >= max_size_) {
            if (NULL != counter) {
                counter->removed(data_.back().map_iterator_->first);
            }
            key2data_.erase(data_.back().map_iterator_);
            data_.pop_back();
        }
        data_.push_front(ListElement(data, it));
        key2data_[key] = data_.begin();
        data_.begin()->map_iterator_ = key2data_.find(key);
    }
    else {
        data_.erase(it->second);
        data_.push_front(ListElement(data, it));
        it->second = data_.begin();
    }
}

} // namespace xscript

#endif // _XSCRIPT_INTERNAL_LRUCACHE_H_
