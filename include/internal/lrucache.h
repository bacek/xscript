#ifndef _XSCRIPT_INTERNAL_LRUCACHE_H_
#define _XSCRIPT_INTERNAL_LRUCACHE_H_

#include <list>
#include <map>
#include <stdexcept>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>

#include "xscript/cache_counter.h"
#include "xscript/profiler.h"
#include "xscript/tag.h"

namespace xscript {

template<typename Data>
struct TagExpiredFunc {
    bool operator() (const Data &element, const Tag &tag) const {
        (void)element;
        return tag.expired();
    }
};

template<typename Key, typename Data, typename ExpireFunc = TagExpiredFunc<Data> >
class LRUCache {
    struct ListElement;
    typedef std::list<ListElement> List;

#ifndef HAVE_HASHMAP
    typedef std::map<Key, typename List::iterator> Map;
#else
    typedef details::hash_map<Key, typename List::iterator> Map;
#endif

public:
    typedef typename Map::iterator iterator;
    typedef typename Map::const_iterator const_iterator;
    
private:
    class ListElement {
    public:
        ListElement(const Data &data, const Tag &tag, const iterator &iter) :
            data_(data), tag_(tag), map_iterator_(iter),
            stored_time_(time(NULL)), prefetch_marked_(false) {}

        Data data_;
        Tag tag_;
        iterator map_iterator_;
        time_t stored_time_;
        bool prefetch_marked_;
    };

    Map key2data_;
    List data_;
    unsigned int max_size_;
    bool check_expire_;
    boost::mutex mutex_;
    ExpireFunc expired_;

    CacheCounter& counter_;
    
public:
    explicit LRUCache(unsigned int size, bool check_expire, CacheCounter &counter);
    ~LRUCache();

    void clear();

    bool load(const Key &key, Data &data, Tag &tag);
    void save(const Key &key, const Data &data, const Tag &tag);
    
private:
    void insert(const Key &key, const Data &data, const Tag &tag, boost::mutex::scoped_lock &lock);

    void erase(iterator it);
    void erase(const Key &key);
    
    bool loadImpl(const Key &key, Data &data, Tag &tag, boost::mutex::scoped_lock &lock);
    bool saveImpl(const Key &key, const Data &data, const Tag &tag, boost::mutex::scoped_lock &lock);
    void push_front(const Key &key, const Data &data, const Tag &tag);
    typename List::iterator getShrinked();
};


template<typename Key, typename Data, typename ExpireFunc>
LRUCache<Key, Data, ExpireFunc>::LRUCache(unsigned int size, bool check_expire, CacheCounter &counter) :
    max_size_(size), check_expire_(check_expire), counter_(counter)
{}

template<typename Key, typename Data, typename ExpireFunc>
LRUCache<Key, Data, ExpireFunc>::~LRUCache() {
}

template<typename Key, typename Data, typename ExpireFunc>
void
LRUCache<Key, Data, ExpireFunc>::erase(iterator it) {
    if (key2data_.end() == it) {
        throw std::out_of_range("invalid iterator in LRUCache");
    }
    data_.erase(it->second);
    key2data_.erase(it);
}

template<typename Key, typename Data, typename ExpireFunc>
void
LRUCache<Key, Data, ExpireFunc>::erase(const Key &key) {
    iterator it = key2data_.find(key);
    erase(it);
}

template<typename Key, typename Data, typename ExpireFunc>
void
LRUCache<Key, Data, ExpireFunc>::insert(
        const Key &key, const Data &data, const Tag &tag, boost::mutex::scoped_lock &lock) {
    
    if (key2data_.empty()) {
        push_front(key, data, tag);
        lock.unlock();
        counter_.incInserted();
        return;
    }
    
    iterator it = key2data_.find(key);
    if (key2data_.end() != it) {
        Data data_tmp = it->second->data_;
        data_.erase(it->second);
        data_.push_front(ListElement(data, tag, it));
        it->second = data_.begin();
        lock.unlock();
        counter_.incUpdated();
        return;
    }

    if (key2data_.size() < max_size_) {
        push_front(key, data, tag);
        lock.unlock();
        counter_.incInserted();
        return;
    }

    typename List::iterator erase_it = getShrinked();    
    Data data_tmp = erase_it->data_;
    key2data_.erase(erase_it->map_iterator_);
    data_.erase(erase_it);
    push_front(key, data, tag);
    
    lock.unlock();
    counter_.incInserted();
}

template<typename Key, typename Data, typename ExpireFunc>
typename LRUCache<Key, Data, ExpireFunc>::List::iterator
LRUCache<Key, Data, ExpireFunc>::getShrinked() {
    typename List::reverse_iterator delete_it = data_.rbegin();
    bool expired_found = false;
    if (check_expire_) {
        for(typename List::reverse_iterator it = data_.rbegin();
            it != data_.rend();
            ++it) {
            if (expired_(it->data_, it->tag_)) {
                expired_found = true;
                delete_it = it;
                break;
            }
        }
    }
    
    if (expired_found) {
        counter_.incExpired();
    }
    else {
        counter_.incExcluded();
    }
    
    return (++delete_it).base();
}

template<typename Key, typename Data, typename ExpireFunc>
void
LRUCache<Key, Data, ExpireFunc>::push_front(const Key &key, const Data &data, const Tag &tag) {
    data_.push_front(ListElement(data, tag, key2data_.end()));
    std::pair<iterator, bool> res = key2data_.insert(std::make_pair(key, data_.begin()));
    data_.begin()->map_iterator_ = res.first;
}

template<typename Key, typename Data, typename ExpireFunc>
bool
LRUCache<Key, Data, ExpireFunc>::load(const Key &key, Data &data, Tag &tag) {
    boost::mutex::scoped_lock lock(mutex_);
    return loadImpl(key, data, tag, lock);
}

template<typename Key, typename Data, typename ExpireFunc>
bool
LRUCache<Key, Data, ExpireFunc>::loadImpl(
        const Key &key, Data &data, Tag &tag, boost::mutex::scoped_lock &lock) {

    iterator it = key2data_.find(key);
    if (key2data_.end() == it) {
        return false;
    }
    
    ListElement& element = *(it->second);
    
    if (expired_(element.data_, element.tag_)) {
        Data data_tmp = element.data_;
        erase(it);
        counter_.incExpired();
        lock.unlock();
        return false;
    }
    
    if (!element.prefetch_marked_ && element.tag_.needPrefetch(element.stored_time_)) {
        element.prefetch_marked_ = true;
        return false;
    }
    
    data = element.data_;
    tag = element.tag_;
    
    counter_.incLoaded();
    
    typename List::iterator list_it = it->second;
    if (data_.begin() != list_it && data_.end() != list_it) {
        data_.splice(data_.begin(), data_, list_it);
        it->second = data_.begin();
    }
    
    return true;
}

template<typename Key, typename Data, typename ExpireFunc>
void
LRUCache<Key, Data, ExpireFunc>::save(const Key &key, const Data &data, const Tag &tag) {
    boost::mutex::scoped_lock lock(mutex_);   
    saveImpl(key, data, tag, lock);
}

template<typename Key, typename Data, typename ExpireFunc>
bool
LRUCache<Key, Data, ExpireFunc>::saveImpl(
        const Key &key, const Data &data, const Tag &tag, boost::mutex::scoped_lock &lock) {
    insert(key, data, tag, lock);
    return true;
}

template<typename Key, typename Data, typename ExpireFunc>
void
LRUCache<Key, Data, ExpireFunc>::clear() {
    Map key2data_tmp;
    List data_tmp;
    boost::mutex::scoped_lock lock(mutex_);
    key2data_.swap(key2data_tmp);
    data_.swap(data_tmp);
}

} // namespace xscript

#endif // _XSCRIPT_INTERNAL_LRUCACHE_H_
