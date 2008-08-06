#include <boost/bind.hpp>
#include "settings.h"
#include "xscript/doc_cache_strategy.h"
#include "xscript/control_extension.h"
#include "xscript/profiler.h"
#include "xscript/tag.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

TagKey::TagKey() {
}

TagKey::~TagKey() {
}

DocCacheStrategy::DocCacheStrategy()
        : statBuilder_("tagged-cache"),
        hitCounter_(AverageCounterFactory::instance()->createCounter("hits")),
        missCounter_(AverageCounterFactory::instance()->createCounter("miss")),
        saveCounter_(AverageCounterFactory::instance()->createCounter("save")) {
    statBuilder_.addCounter(hitCounter_.get());
    statBuilder_.addCounter(missCounter_.get());
    statBuilder_.addCounter(saveCounter_.get());
}

DocCacheStrategy::~DocCacheStrategy() {
}

void DocCacheStrategy::init(const Config *config) {
    (void)config;
}



bool DocCacheStrategy::loadDoc(const TagKey *key, Tag &tag, XmlDocHelper &doc) {

    //return loadDocImpl(key, tag, doc);

    boost::function<bool ()> f = boost::bind(
                                     &DocCacheStrategy::loadDocImpl,
                                     this, boost::cref(key), boost::ref(tag), boost::ref(doc)
                                 );
    std::pair<bool, uint64_t> res = profile(f);

    if (res.first)
        hitCounter_->add(res.second);
    else
        missCounter_->add(res.second);
    return res.first;
}

bool DocCacheStrategy::saveDoc(const TagKey *key, const Tag& tag, const XmlDocHelper &doc) {
    boost::function<bool ()> f = boost::bind(
                                     &DocCacheStrategy::saveDocImpl,
                                     this, boost::cref(key), boost::ref(tag), boost::ref(doc)
                                 );
    std::pair<bool, uint64_t> res = profile(f);
    saveCounter_->add(res.second);
    return res.first;
}

} // namespace xscript
