#ifndef _XSCRIPT_INTERNAL_LRUCACHE_H_
#define _XSCRIPT_INTERNAL_LRUCACHE_H_

#include <stdexcept>
#include "xscript/cache_usage_counter.h"

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

    std::auto_ptr<CacheCounter> counter_;
    
public:
    explicit LRUCache(unsigned int size, bool check_expire, const std::string &name);
    ~LRUCache();

    void clear();

    bool load(const Key &key, Data &data, const Tag &tag);
    void save(const Key &key, const Data &data, const Tag &tag);
    
    const CounterBase* counter() const;
    
private:
    iterator fetch(const Key &key);
    void insert(const Key &key, const Data &data, const Tag &tag, boost::mutex::scoped_lock &lock);

    void erase(iterator it);
    void erase(const Key &key);
    
    bool loadImpl(const Key &key, Data &data, const Tag &tag, boost::mutex::scoped_lock &lock);
    void saveImpl(const Key &key, const Data &data, const Tag &tag, boost::mutex::scoped_lock &lock);
    void push_front(const Key &key, Data &data, const Tag &tag);
};


template<typename Key, typename Data, typename ExpireFunc>
LRUCache<Key, Data, ExpireFunc>::LRUCache(unsigned int size, bool check_expire, const std::string &name) :
    max_size_(size), check_expire_(check_expire),
    counter_(CacheCounterFactory::instance()->createCounter(name))
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
typename LRUCache<Key, Data, ExpireFunc>::iterator
LRUCache<Key, Data, ExpireFunc>::fetch(const Key &key) {
    iterator map_it = key2data_.find(key);
    if (key2data_.end() != map_it) {
        typename List::iterator list_it = map_it->second;
        if (data_.begin() != list_it && data_.end() != list_it) {
            data_.splice(data_.begin(), data_, list_it);
            map_it->second = data_.begin();
	}
    }
    return map_it;
}

template<typename Key, typename Data, typename ExpireFunc>
void
LRUCache<Key, Data, ExpireFunc>::insert(
        const Key &key, const Data &data, const Tag &tag, boost::scoped_lock &lock) {
    
    if (key2data_.empty()) {
        push_front(key, data, tag);
        return;
    }
    
    iterator it = key2data_.find(key);
    if (key2data_.end() != it) {
        Data data_tmp = it->second->data_;
        data_.erase(it->second);
        data_.push_front(ListElement(data, tag, it));
        it->second = data_.begin();
        lock.unlock();
        return;
    }

    if (key2data_.size() < max_size_) {
        push_front(key, data, tag);
        return;
    }
    
    List::reverse_iterator delete_it = data_.rbegin();
    
    bool expired_found = false;
    if (check_expire_) {
        for(List::reverse_iterator it = data_.rbegin();
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
        counter_->incExpired();
    }
    else {
        counter_->incExcluded();
    }
    
    Data data_tmp = delete_it->data_;
    key2data_.erase(delete_it->map_iterator_);
    data_.erase(delete_it);

    push_front(key, data, tag);

    lock.unlock();
}

template<typename Key, typename Data, typename ExpireFunc>
void
LRUCache<Key, Data, ExpireFunc>::push_front(const Key &key, Data &data, const Tag &tag) {
    data_.push_front(ListElement(data, tag, key2data_.end()));
    std::pair<iterator, bool> res = key2data_.insert(std::make_pair(key, data_.begin()));
    data_.begin()->map_iterator_ = res.first;
}

template<typename Key, typename Data, typename ExpireFunc>
bool
LRUCache<Key, Data, ExpireFunc>::load(const Key &key, Data &data, const Tag &tag) {
    boost::mutex::scoped_lock lock(mutex_);
    
    boost::function<bool()> f = boost::bind(&LRUCache<Key, Data, ExpireFunc>::loadImpl,
            this, boost::cref(key), boost::ref(data), boost::cref(tag));
    
    std::pair<bool, uint64_t> res = profile(f);
    
    if (res.first) {
        counter_->addHit(res.second);
    }
    else {
        counter_->addMiss(res.second);
    }
    
    return res.first;
}

template<typename Key, typename Data, typename ExpireFunc>
bool
LRUCache<Key, Data, ExpireFunc>::loadImpl(
        const Key &key, Data &data, const Tag &tag, boost::mutex::scoped_lock &lock) {

    iterator it = find(key);
    if (key2data_.end() == it) {
        return false;
    }
    
    ListElement& element = *(it->second);
    
    if (expired_(element.data_, element.tag_)) {
        Data data_tmp = element.data_;
        erase(it);
        counter_->incExpired();
        lock.unlock();
        return false;
    }
    
    if (!element.prefetch_marked_ && element.tag_.needPrefetch(element.stored_time_)) {
        element.prefetch_marked_ = true;
        return false;
    }
    
    data = element.data_;
    tag = element.tag_;
    
    counter_->incLoaded();
    
    return true;
}

template<typename Key, typename Data, typename ExpireFunc>
void
LRUCache<Key, Data, ExpireFunc>::save(const Key &key, const Data &data, const Tag &tag) {
    boost::mutex::scoped_lock lock(mutex_);
    
    boost::function<void()> f = boost::bind(&LRUCache<Key, Data, ExpireFunc>::saveImpl,
            this, boost::cref(key), boost::cref(data), boost::cref(tag), boost::ref(lock));
    
    std::pair<void, uint64_t> res = profile(f);
    counter_->addSave(res.second);
}

template<typename Key, typename Data, typename ExpireFunc>
void
LRUCache<Key, Data, ExpireFunc>::saveImpl(
        const Key &key, const Data &data, const Tag &tag, boost::mutex::scoped_lock &lock) {
    insert(key, data, tag, lock);
    counter_->incStored();
}

template<typename Key, typename Data, typename ExpireFunc>
void
LRUCache<Key, Data, ExpireFunc>::clear() {
    boost::mutex::scoped_lock lock(mutex_);
    Map key2data_tmp;
    List data_tmp;
    key2data_.swap(key2data_tmp);
    data_.swap(data_tmp);
    lock.unlock();
}

template<typename Key, typename Data, typename ExpireFunc>
const CounterBase*
LRUCache<Key, Data, ExpireFunc>::counter() const {
    return counter_.get();
}

} // namespace xscript

#endif // _XSCRIPT_INTERNAL_LRUCACHE_H_
