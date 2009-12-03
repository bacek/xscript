#include "settings.h"

#include <boost/current_function.hpp>

#include "xscript/average_counter.h"
#include "xscript/logger.h"
#include "xscript/xml_util.h"

#include "doc_pool.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

DocPool::DocPool(size_t capacity, const std::string& name) :
        capacity_(capacity),
        counter_(CacheCounterFactory::instance()->createCounter(name)) {
}

DocPool::~DocPool() {
    clear();
}

const CacheCounter*
DocPool::getCounter() const {
    return counter_.get();
}

bool
DocPool::loadDoc(const TagKey &key, Tag &tag, XmlDocSharedHelper &doc) {
    const std::string &keyStr = key.asString();
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
DocPool::loadDocImpl(const std::string &keyStr, Tag &tag, XmlDocSharedHelper &doc) {
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
        counter_->incRemoved();
        data.clearDoc();
        key2data_.erase(i);
        return LOAD_EXPIRED;
    }
    if (!data.prefetch_marked && data.tag.needPrefetch(data.stored_time)) {
        data.prefetch_marked = true;
        return LOAD_NEED_PREFETCH;
    }

    tag = data.tag;
    doc = data.doc;

    if (data.pos != list_.end()) {
        list_.erase(data.pos);
    }
    data.pos = list_.insert(list_.end(), i);

    counter_->incLoaded();
    return LOAD_SUCCESSFUL;
}

bool
DocPool::saveDoc(const TagKey &key, const Tag& tag, const XmlDocSharedHelper &doc) {
    const std::string &keyStr = key.asString();
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
DocPool::saveDocImpl(const std::string &keyStr, const Tag& tag, const XmlDocSharedHelper &doc) {

    log()->debug("%s, key: %s", BOOST_CURRENT_FUNCTION, keyStr.c_str());

    boost::mutex::scoped_lock lock(mutex_);

    counter_->incStored();

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


void
DocPool::saveAtIterator(const Key2Data::iterator &i, const Tag &tag, const XmlDocSharedHelper &doc) {
    DocData &data = i->second;

    if (data.pos != list_.end()) {
        list_.erase(data.pos);
    }
    data.assign(tag, doc);
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
            counter_->incRemoved();
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
            counter_->incRemoved();
            data.clearDoc();
            key2data_.erase(i);
            list_.erase(li++);
        }
        else {
            ++li;
        }
    }
}



DocPool::DocData::DocData() : tag(), pos(), prefetch_marked(false) {
}

DocPool::DocData::DocData(LRUList::iterator list_pos) :
        tag(), pos(list_pos), prefetch_marked(false) {
}

void
DocPool::DocData::assign(const Tag &t, const XmlDocSharedHelper &elem) {
    assert(NULL != elem.get());
    assert(NULL != elem->get());
    clearDoc();

    tag = t;
    doc = elem;

    stored_time = time(NULL);
    prefetch_marked = false;
}

void
DocPool::DocData::clearDoc() {
    doc.reset();
}

} // namespace xscript
