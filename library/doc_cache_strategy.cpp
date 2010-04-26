#include "settings.h"

#include <boost/bind.hpp>
#include <boost/tokenizer.hpp>

#include "xscript/config.h"
#include "xscript/control_extension.h"
#include "xscript/doc_cache.h"
#include "xscript/doc_cache_strategy.h"
#include "xscript/logger.h"
#include "xscript/tag.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

TagKey::TagKey() {
}

TagKey::~TagKey() {
}

DocCacheStrategy::DocCacheStrategy() {
}

DocCacheStrategy::~DocCacheStrategy() {
}

void
DocCacheStrategy::init(const Config *config) {
    (void)config;
}

void
DocCacheStrategy::insert2Cache(const std::string &no_cache) {

    typedef boost::char_separator<char> Separator;
    typedef boost::tokenizer<Separator> Tokenizer;

    bool cache_block = true, cache_page = true;
    Tokenizer tok(no_cache, Separator(" ,"));
    for (Tokenizer::iterator it = tok.begin(), end = tok.end(); it != end; ++it) {
        if (strcasecmp(no_cache.c_str(), "page") == 0) {
            cache_page = false;
        }
        else if (strcasecmp(no_cache.c_str(), "block") == 0) {
            cache_block = false;
        }
    }
        
    if (cache_page) {
        PageCache::instance()->addStrategy(this, name());
    }
    if (cache_block) {
        DocCache::instance()->addStrategy(this, name());
    }
}

void
DocCacheStrategy::fillStatBuilder(StatBuilder *builder) {
    (void)builder;
}

} // namespace xscript
