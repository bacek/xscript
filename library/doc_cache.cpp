#include "settings.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

#include "xscript/algorithm.h"
#include "xscript/average_counter.h"
#include "xscript/context.h"
#include "xscript/doc_cache.h"
#include "xscript/doc_cache_strategy.h"
#include "xscript/http_utils.h"
#include "xscript/logger.h"
#include "xscript/profiler.h"
#include "xscript/stat_builder.h"
#include "xscript/tag.h"
#include "xscript/tagged_block.h"
#include "xscript/tagged_cache_usage_counter.h"
#include "xscript/xml_util.h"

#include "internal/parser.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class DocCacheBase::StatInfo {
public:
    std::auto_ptr<StatBuilder> statBuilder_;
    std::auto_ptr<AverageCounter> hitCounter_;
    std::auto_ptr<AverageCounter> missCounter_;
    std::auto_ptr<AverageCounter> saveCounter_;
    std::auto_ptr<TaggedCacheUsageCounter> usageCounter_;
};

class DocCacheBase::DocCacheData {
public:
    DocCacheData(DocCacheBase *owner);
    ~DocCacheData();

    std::auto_ptr<Block> createBlock(const ControlExtension *ext, Xml *owner, xmlNodePtr node);
    XmlDocHelper createReport() const;

    class DocCacheBlock : public Block {
    public:
        DocCacheBlock(const ControlExtension *ext, Xml *owner, xmlNodePtr node, const DocCacheData &cache_data)
                : Block(ext, owner, node), cache_data_(cache_data) {
        }

        void call(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const throw (std::exception) {
            ControlExtension::setControlFlag(ctx.get());
            invoke_ctx->resultDoc(cache_data_.createReport());
        }
    private:
        const DocCacheData &cache_data_;
    };
    
    DocCacheBase *owner_;
    std::vector<std::string> strategiesOrder_;
    typedef std::vector<std::pair<DocCacheStrategy*, boost::shared_ptr<StatInfo> > > StrategyMap;
    StrategyMap strategies_;
};

DocCacheBase::DocCacheData::DocCacheData(DocCacheBase *owner) : owner_(owner)
{}

DocCacheBase::DocCacheData::~DocCacheData()
{}

std::auto_ptr<Block>
DocCacheBase::DocCacheData::createBlock(const ControlExtension *ext, Xml *owner, xmlNodePtr node) {
    return std::auto_ptr<Block>(new DocCacheBlock(ext, owner, node, *this));
}

XmlDocHelper
DocCacheBase::DocCacheData::createReport() const {
    XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != doc.get());

    xmlNodePtr root = xmlNewDocNode(doc.get(), NULL, (const xmlChar*)owner_->name().c_str(), NULL);
    XmlUtils::throwUnless(NULL != root);

    xmlDocSetRootElement(doc.get(), root);

    // Sorted list of strategies
    for(DocCacheData::StrategyMap::const_iterator s = strategies_.begin();
        s != strategies_.end();
        ++s) {
        XmlDocHelper report = s->second->statBuilder_->createReport();
        XmlNodeHelper node(xmlCopyNode(xmlDocGetRootElement(report.get()), 1));
        boost::uint64_t hits = s->second->hitCounter_->count();
        boost::uint64_t misses = s->second->missCounter_->count();
        if (hits > 0 || misses > 0) {
            double hit_ratio = (double)hits/((double)hits + (double)misses);
            xmlNewProp(node.get(), (const xmlChar*)"hit-ratio",
                (const xmlChar*)boost::lexical_cast<std::string>(hit_ratio).c_str());
        }
        xmlAddChild(root, node.get());
        node.release();
    }

    return doc;
}

DocCacheBase::DocCacheBase() : data_(new DocCacheData(this)) {
}

DocCacheBase::~DocCacheBase() {
}

void
DocCacheBase::init(const Config *config) {
    ControlExtension::Constructor f = boost::bind(boost::mem_fn(&DocCacheData::createBlock), data_.get(), _1, _2, _3);
    ControlExtension::registerConstructor(name().append("-stat"), f);
    
    TaggedCacheUsageCounterFactory::instance()->init(config);
    
    for(DocCacheData::StrategyMap::iterator i = data_->strategies_.begin();
        i != data_->strategies_.end();
        ++i) {
        std::string builder_name = name() + "-" + i->first->name();
        std::auto_ptr<StatBuilder> builder(new StatBuilder(builder_name));
        boost::shared_ptr<StatInfo> stat_info(new StatInfo());
        stat_info->hitCounter_ = AverageCounterFactory::instance()->createCounter("hits");
        stat_info->missCounter_ = AverageCounterFactory::instance()->createCounter("miss");
        stat_info->saveCounter_ = AverageCounterFactory::instance()->createCounter("save");
        
        createUsageCounter(stat_info);
        
        builder->addCounter(stat_info->hitCounter_.get());
        builder->addCounter(stat_info->missCounter_.get());
        builder->addCounter(stat_info->saveCounter_.get());
        if (stat_info->usageCounter_.get()) {
            builder->addCounter(stat_info->usageCounter_.get());
        }
        
        i->first->fillStatBuilder(builder.get());
        
        stat_info->statBuilder_ = builder;        
        i->second = stat_info;
    }
}

void
DocCacheBase::addStrategy(DocCacheStrategy *strategy, const std::string &name) {
    // FIXME Add into proper position
    (void)name;
    log()->debug("DocCacheBase::addStrategy %s", name.c_str());
    data_->strategies_.push_back(std::make_pair(strategy, boost::shared_ptr<StatInfo>()));
}

time_t
DocCacheBase::minimalCacheTime() const {
    // FIXME.Do we actually need this in public part? Better to embed logic
    // of minimalCacheTime into saveDoc.
    return 5;
}


static std::string
createLogInfo(const Context *ctx, const TaggedBlock *block) {

    std::string info;
    if (ctx) {
        if (block) {
            info.append(" Block info: ").append(block->info(ctx, true));
        }
        info.append(" Url: ").append(ctx->rootContext()->request()->getOriginalUrl());
    }
    return info;
}

bool
DocCacheBase::checkTag(const Context *ctx, const TaggedBlock *block, const Tag &tag, const char *operation) {

    if (tag.valid()) {
        return true;
    }

    if (log()->enabledWarn()) {
        std::string log_info = createLogInfo(ctx, block);
        log()->warn("Tag is not valid while %s.%s", operation, log_info.c_str());
    }
    return false;
}

bool
DocCacheBase::allow(const DocCacheStrategy* strategy, const CacheContext *cache_ctx) const {
    CachedObject::Strategy cache_strategy = strategy->strategy();
    if (CachedObject::UNKNOWN == cache_strategy) {
        return false;
    }
    
    //TODO: move to checkStrategy;
    if (CachedObject::DISTRIBUTED == cache_strategy) {
        return cache_ctx->allowDistributed();
    }
    
    return cache_ctx->object()->checkStrategy(cache_strategy);
}

bool
DocCacheBase::loadDocImpl(InvokeContext *invoke_ctx, CacheContext *cache_ctx,
    Tag &tag, boost::shared_ptr<CacheData> &cache_data) {

    log()->entering("DocCacheBase::loadDocImpl");
    
    typedef std::vector<std::pair<
        DocCacheData::StrategyMap::iterator, boost::shared_ptr<TagKey> > > KeyMapType;
    KeyMapType missed_keys;
    
    bool loaded = false;
    bool check_tag_key = NULL != invoke_ctx;
    Context *ctx = cache_ctx->context();
    CachedObject* object = cache_ctx->object();
    DocCacheData::StrategyMap::iterator i = data_->strategies_.begin();
    while(!loaded && i != data_->strategies_.end()) {
        DocCacheStrategy* strategy = i->first;
        if (!allow(strategy, cache_ctx)) {
            ++i;
            continue;
        }
        
        boost::shared_ptr<TagKey> key(strategy->createKey(ctx, invoke_ctx, object).release());
        
        boost::function<bool()> f = boost::bind(&DocCacheStrategy::loadDoc,
                strategy, boost::cref(key.get()), cache_ctx, boost::ref(tag), boost::ref(cache_data));
        
        std::pair<bool, uint64_t> res = profile(f);
        
        if (res.first) {
            if (i->second->usageCounter_.get()) {
                i->second->usageCounter_->fetchedHit(ctx, object);
            }
            i->second->hitCounter_->add(res.second);
        }
        else {
            if (i->second->usageCounter_.get()) {
                i->second->usageCounter_->fetchedMiss(ctx, object);
            }
            i->second->missCounter_->add(res.second);
            missed_keys.push_back(std::make_pair(i, key));
        }
        
        if (check_tag_key) {
            check_tag_key = false;
            invoke_ctx->tagKey(key);
        }
        
        loaded = res.first;
        ++i;
    }

    if (!loaded) {
        const TaggedBlock *block = NULL == invoke_ctx ? NULL : dynamic_cast<const TaggedBlock*>(object);
        checkTag(ctx, block, tag, "loading doc from tagged cache");
    }
    else {
        for(KeyMapType::iterator it = missed_keys.begin();
            it != missed_keys.end();
            ++it) {
            DocCacheData::StrategyMap::iterator iter = it->first;
            TagKey* key = it->second.get();
            boost::function<bool()> f = boost::bind(&DocCacheStrategy::saveDoc, iter->first,
                boost::cref(key), cache_ctx, boost::cref(tag), boost::cref(cache_data));
            std::pair<bool, uint64_t> res = profile(f);
            iter->second->saveCounter_->add(res.second);  
        }
    }
    
    return loaded;
}

bool
DocCacheBase::saveDocImpl(const InvokeContext *invoke_ctx, CacheContext *cache_ctx,
    const Tag &tag, const boost::shared_ptr<CacheData> &cache_data) {
    
    log()->entering("DocCacheBase::saveDocImpl");

    Context *ctx = cache_ctx->context();
    CachedObject *object = cache_ctx->object();

    TaggedBlock *block = NULL;
    bool check_tag_key = NULL != invoke_ctx;
    if (check_tag_key) {
        block = dynamic_cast<TaggedBlock*>(object);
        if (NULL == block) {
            throw std::logic_error("NULL block while saving it to cache");
        }
        xmlDocPtr doc = cache_data->docPtr();
        if (NULL == doc || NULL == xmlDocGetRootElement(doc)) {
            if (log()->enabledWarn()) {
                std::string log_info = createLogInfo(ctx, block);
                log()->warn("Cannot save empty document or document with no root to tagged cache.%s",
                    log_info.c_str());
            }
            return false;
        }
    }
    if (!checkTag(ctx, block, tag, "saving doc to tagged cache")) {
        return false;
    }
    
    bool saved = false;
    for (DocCacheData::StrategyMap::iterator i = data_->strategies_.begin();
         i != data_->strategies_.end();
         ++i) {
        DocCacheStrategy* strategy = i->first;
        if (!allow(strategy, cache_ctx)) {
            continue;
        }
        std::auto_ptr<TagKey> key = strategy->createKey(ctx, invoke_ctx, object);

        if (check_tag_key) {
            check_tag_key = false;
            TagKey* load_key = invoke_ctx->tagKey();            
            if (load_key && key->asString() != load_key->asString()) {
                if (log()->enabledWarn()) {
                    std::string log_info = createLogInfo(ctx, block);
                    log()->warn("Cache key modified during call.%s", log_info.c_str());
                }
                block->tagged(false);
                return false;
            }
        }
        
        boost::function<bool()> f =
            boost::bind(&DocCacheStrategy::saveDoc, strategy,
                    boost::cref(key.get()), cache_ctx, boost::cref(tag), boost::cref(cache_data));
        
        std::pair<bool, uint64_t> res = profile(f);
        i->second->saveCounter_->add(res.second); 
        
        saved |= res.first;
    }
    return saved;
}

DocCache::DocCache() {
}

DocCache::~DocCache() {
}

DocCache*
DocCache::instance() {
    static DocCache cache;
    return &cache;
}

void
DocCache::createUsageCounter(boost::shared_ptr<StatInfo> info) {
    info->usageCounter_ = TaggedCacheUsageCounterFactory::instance()->createBlockCounter("disgrace");
}

std::string
DocCache::name() const {
    return "block-cache";
}

boost::shared_ptr<BlockCacheData>
DocCache::loadDoc(InvokeContext *invoke_ctx, CacheContext *cache_ctx, Tag &tag) {
    boost::shared_ptr<CacheData> cache_data(new BlockCacheData());
    bool res = loadDocImpl(invoke_ctx, cache_ctx, tag, cache_data);
    if (!res) {
        return boost::shared_ptr<BlockCacheData>();
    }
    log()->info("Doc loaded from cache");
    return boost::dynamic_pointer_cast<BlockCacheData>(cache_data);
}

bool
DocCache::saveDoc(const InvokeContext *invoke_ctx, CacheContext *cache_ctx,
    const Tag &tag, const boost::shared_ptr<BlockCacheData> &cache_data) {
    if (saveDocImpl(invoke_ctx, cache_ctx, tag, boost::dynamic_pointer_cast<CacheData>(cache_data))) {
        log()->info("Doc saved to cache");
        return true;
    }
    return false;
}

PageCache::PageCache() : use_etag_(false) {
}

PageCache::~PageCache() {
}

PageCache*
PageCache::instance() {
    static PageCache cache;
    return &cache;
}

void
PageCache::init(const Config *config) {
    use_etag_ = config->as<bool>("/xscript/page-cache-strategies/use-etag", false);
    DocCacheBase::init(config);
}

bool
PageCache::useETag() const {
    return use_etag_;
}

void
PageCache::createUsageCounter(boost::shared_ptr<StatInfo> info) {
    info->usageCounter_ = TaggedCacheUsageCounterFactory::instance()->createScriptCounter("disgrace");
}

std::string
PageCache::name() const {
    return "page-cache";
}

boost::shared_ptr<PageCacheData>
PageCache::loadDoc(CacheContext *cache_ctx, Tag &tag) {
    boost::shared_ptr<CacheData> cache_data(new PageCacheData());
    bool res = loadDocImpl(NULL, cache_ctx, tag, cache_data);
    if (!res) {
        return boost::shared_ptr<PageCacheData>();
    }
    return boost::dynamic_pointer_cast<PageCacheData>(cache_data);
}

bool
PageCache::saveDoc(CacheContext *cache_ctx, const Tag &tag,
        const boost::shared_ptr<PageCacheData> &cache_data) {
    return saveDocImpl(NULL, cache_ctx, tag, boost::dynamic_pointer_cast<CacheData>(cache_data));
}

CacheContext::CacheContext(CachedObject *obj, Context *ctx) :
    obj_(obj), ctx_(ctx), allow_distributed_(true)
{}

CacheContext::CacheContext(CachedObject *obj, Context *ctx, bool allow_distributed) :
    obj_(obj), ctx_(ctx), allow_distributed_(allow_distributed)
{}

CachedObject*
CacheContext::object() const {
    return obj_;
}

Context*
CacheContext::context() const {
    return ctx_;
}

bool
CacheContext::allowDistributed() const {
    return allow_distributed_;
}

void
CacheContext::allowDistributed(bool flag) {
    allow_distributed_ = flag;
}

CacheData::CacheData()
{}

CacheData::~CacheData()
{}

PageCacheData::PageCacheData() : expire_time_delta_(0)
{}

PageCacheData::PageCacheData(const char *buf, std::streamsize size) :
    data_(buf, size), expire_time_delta_(0)
{}

PageCacheData::~PageCacheData()
{}
       
void
PageCacheData::append(const char *buf, std::streamsize size) {
    data_.append(buf, size);
}

void
PageCacheData::addHeader(const std::string &name, const std::string &value) {
    headers_.push_back(std::make_pair(name, value));
}

void
PageCacheData::expireTimeDelta(boost::uint32_t delta) {
    expire_time_delta_ = delta;
}

xmlDocPtr
PageCacheData::docPtr() const {
    return NULL;
}

bool
PageCacheData::parse(const char *buf, boost::uint32_t size) {
    if (size < 3*sizeof(boost::uint32_t)) {
        log()->error("error while parsing page cache data: incorrect length");
        return false;
    }
    
    boost::uint32_t sign = *((boost::uint32_t*)buf);
    if (SIGNATURE != sign) {
        log()->error("error while parsing page cache data: incorrect sign: %d", sign);
        return false;
    }
    
    data_.clear();
    headers_.clear();
    
    const char *buf_orig = buf;

    buf += sizeof(boost::uint32_t);
    expire_time_delta_ = *((boost::uint32_t*)buf);
    buf += sizeof(boost::uint32_t);
    
    boost::uint32_t etag_size = *((boost::uint32_t*)buf);
    buf += sizeof(boost::uint32_t);

    if (buf - buf_orig + etag_size >= size) {
        log()->error("error while parsing page cache data: incorrect etag length");
        return false;
    }

    etag_.assign(buf, etag_size);
    buf += etag_size;

    Range data(buf, buf_orig + size);
    while(!data.empty()) {
        Range header;
        split(data, Parser::RN_RANGE, header, data);
        if (header.empty()) {
            data_.assign(data.begin(), data.size());
            break;
        }
        
        Range name, value;
        split(header, ':', name, value);
        if (name.empty()) {
            log()->warn("Empty header name while parsing page cache data");
            continue;
        }
        
        headers_.push_back(std::make_pair(
            std::string(name.begin(), name.size()),
            std::string(value.begin(), value.size())));
    }
    
    return true;
}

bool
PageCacheData::serialize(std::string &buf) {

    checkETag();
    boost::uint32_t etag_size = etag_.size();

    size_t sz = 0;
    for(std::vector<std::pair<std::string, std::string> >::iterator i = headers_.begin();
        i != headers_.end(); ++i) {
        sz += i->first.size() + i->second.size() + 3;
    }

    buf.clear();
    buf.reserve(sizeof(SIGNATURE) + sizeof(expire_time_delta_) + sizeof(etag_size) + etag_size + sz + 2 + data_.size());

    buf.append((const char*)&SIGNATURE, sizeof(SIGNATURE));
    buf.append((const char*)&expire_time_delta_, sizeof(expire_time_delta_));

    buf.append((char*)&etag_size, sizeof(etag_size));
    buf.append(etag_);

    for(std::vector<std::pair<std::string, std::string> >::iterator it = headers_.begin();
        it != headers_.end();
        ++it) {
        buf.append(it->first);
        buf.push_back(':');
        buf.append(it->second);
        buf.append("\r\n", 2);
    }
    buf.append("\r\n", 2);
    buf.append(data_);
    return true;
}

void
PageCacheData::cleanup(Context *ctx) {
    (void)ctx;
}

const std::string&
PageCacheData::etag() const {
    return etag_;
}

void
PageCacheData::checkETag() const {
    if (!etag_.empty()) {
        return;
    }
    std::string key;
    typedef std::vector<std::pair<std::string, std::string> > HeaderMap;
    for (HeaderMap::const_iterator it = headers_.begin(), end = headers_.end();
        it != end;
        ++it) {
        key.append(it->first);
        key.append(it->second);
    }
    key.append(HashUtils::hexMD5(data_.c_str(), data_.size()));
    etag_.reserve(2 + 32);
    etag_.push_back('"');
    etag_.append(HashUtils::hexMD5(key.c_str(), key.size()));
    etag_.push_back('"');
}

void
PageCacheData::write(std::ostream *os, const Response *response) const {
    checkETag();
    typedef std::vector<std::pair<std::string, std::string> > HeaderMap;
    for (HeaderMap::const_iterator it = headers_.begin(), end = headers_.end();
        it != end;
        ++it) {       
        (*os) << it->first << ": " << it->second << "\r\n";
    }
    
    const CookieSet& cookies = response->outCookies();
    for (CookieSet::const_iterator it = cookies.begin(), end = cookies.end(); it != end; ++it) {
        (*os) << "Set-Cookie: " << it->toString() << "\r\n";
    }

    if (PageCache::instance()->useETag()) {
        (*os) << "ETag: " << etag_ << "\r\n";
    }

    boost::int32_t expires = HttpDateUtils::expires(expire_time_delta_);
    (*os) << "Expires: " << HttpDateUtils::format(expires) << "\r\n\r\n";
    
    os->write(data_.c_str(), data_.size());
}

bool
PageCacheData::notModified(Context *ctx) const {
    if (!PageCache::instance()->useETag()) {
        return false;
    }
    const std::string& if_none_match =
        ctx->request()->getHeader(HttpUtils::IF_NONE_MATCH_HEADER_NAME);
    if (if_none_match.empty()) {
        return false;
    }
    bool matched = false;
    typedef boost::char_separator<char> Separator;
    typedef boost::tokenizer<Separator> Tokenizer;
    Tokenizer tok(if_none_match, Separator(", "));
    for (Tokenizer::iterator it = tok.begin(), it_end = tok.end(); it != it_end; ++it) {
        if (*it == etag_) {
            matched = true;
            break;
        }
    }
    return matched;
}

std::streamsize
PageCacheData::size() const {
    return 0;
}

const boost::uint32_t BlockCacheData::SIGNATURE = 0xffffff3a;
const boost::uint32_t PageCacheData::SIGNATURE = 0xffffff1b;

BlockCacheData::BlockCacheData()
{}

BlockCacheData::BlockCacheData(XmlDocSharedHelper doc, boost::shared_ptr<MetaCore> meta) :
        doc_(doc), meta_(meta)
{}

BlockCacheData::~BlockCacheData()
{}

const XmlDocSharedHelper&
BlockCacheData::doc() const {
    return doc_;
}

const boost::shared_ptr<MetaCore>&
BlockCacheData::meta() const {
    return meta_;
}

xmlDocPtr
BlockCacheData::docPtr() const {
    return doc_.get();
}

bool
BlockCacheData::parse(const char *buf, boost::uint32_t size) {
    try {
        if (size < 2*sizeof(boost::uint32_t)) {
            log()->error("error while parsing block cache data: incorrect length");
            return false;
        }
        
        boost::uint32_t sign = *((boost::uint32_t*)buf);
        if (SIGNATURE != sign) {
            log()->error("error while parsing block cache data: incorrect sign: %d", sign);
            return false;
        }
        
        buf += sizeof(boost::uint32_t);
        size -= sizeof(boost::uint32_t);
        
        boost::uint32_t meta_size = *((boost::uint32_t*)buf);
        buf += sizeof(boost::uint32_t);
        size -= sizeof(boost::uint32_t);
        if (meta_size > size) {
            log()->error("error while parsing block cache data: incorrect meta length");
            return false;
        }

        Range meta_data(buf, buf + meta_size);
        if (!meta_data.empty()) {
            meta_.reset(new MetaCore());
            meta_->parse(meta_data.begin(), meta_data.size());
        }

        if (meta_size == size) {
            return true;
        }

        XmlDocHelper newdoc(xmlReadMemory(buf + meta_size, size - meta_size, "",
            "UTF-8", XML_PARSE_DTDATTR | XML_PARSE_NOENT));
        XmlUtils::throwUnless(NULL != newdoc.get());
        if (NULL == xmlDocGetRootElement(newdoc.get())) {
            log()->warn("get document with no root while parsing block cache data");
            return false;
        }
        doc_.reset(newdoc.release());
        return true;
    }
    catch (const std::exception &e) {
        log()->error("error while parsing block cache data: %s", e.what());
        return false;
    }
}

extern "C" int
cacheWriteFunc(void *ctx, const char *data, int len) {
    std::string* str = static_cast<std::string*>(ctx);
    try {
        str->append(data, len);
        return len;
    }
    catch (const std::exception &e) {
        log()->error("caught exception while writing cache data: %s", e.what());
    }
    return -1;
}

extern "C" int
cacheCloseFunc(void *ctx) {
    (void) ctx;
    return 0;
}

bool
BlockCacheData::serialize(std::string &buf) {
    buf.clear();
    buf.append((char*)&SIGNATURE, sizeof(SIGNATURE));
    boost::uint32_t meta_size = 0;
    buf.append((char*)&meta_size, sizeof(meta_size));
    if (meta_.get()) {
        meta_->serialize(buf);
    }
    meta_size = buf.size() - sizeof(SIGNATURE) - sizeof(meta_size);
    if (meta_size > 0) {
        buf.replace(sizeof(SIGNATURE), sizeof(meta_size),
            (char*)&meta_size, sizeof(meta_size));
    }
    xmlOutputBufferPtr buffer = NULL;
    buffer = xmlOutputBufferCreateIO(&cacheWriteFunc, &cacheCloseFunc, &buf, NULL);
    xmlSaveFormatFileTo(buffer, doc_.get(), "UTF-8", 0);
    return true;
}

void
BlockCacheData::cleanup(Context *ctx) {
    ctx->addDoc(doc_);
}

} // namespace xscript
