#include "settings.h"

#include <cerrno>
#include <fstream>
#include <sstream>

#include <boost/crc.hpp>
#include <boost/cstdint.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <libxml/xmlIO.h>

#include "xscript/tag.h"
#include "xscript/util.h"
#include "xscript/string_utils.h"
#include "xscript/xml_util.h"
#include "xscript/param.h"
#include "xscript/tagged_block.h"
#include "xscript/logger.h"
#include "xscript/config.h"
#include "xscript/context.h"
#include "xscript/doc_cache_strategy.h"
#include "xscript/doc_cache.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class TaggedKeyDisk : public TagKey {
public:
    TaggedKeyDisk(const Context *ctx, const CachedObject *obj);

    boost::uint32_t number() const;
    virtual const std::string& asString() const;
    const std::string& filename() const;

private:
    std::string value_;
    std::string filename_;
    boost::uint32_t number_;
};

class DocCacheDisk :
            public Component<DocCacheDisk>,
            public DocCacheStrategy {
public:
    DocCacheDisk();
    virtual ~DocCacheDisk();

    virtual void init(const Config *config);

    virtual time_t minimalCacheTime() const;
    virtual std::string name() const;

    virtual std::auto_ptr<TagKey> createKey(const Context *ctx, const CachedObject *obj) const;

protected:
    virtual bool loadDocImpl(const TagKey *key, Tag &tag, XmlDocSharedHelper &doc, bool need_copy);
    virtual bool saveDocImpl(const TagKey *key, const Tag &tag, const XmlDocSharedHelper &doc, bool need_copy);
    
private:
    static void makeDir(const std::string &name);
    static void createDir(const std::string &name);

    static bool load(const std::string &path, const std::string &key, Tag &tag, std::vector<char> &doc_data);
    static bool save(const std::string &path, const std::string &key, const Tag &tag, xmlDocPtr doc);


    static const time_t DEFAULT_CACHE_TIME;
    static const boost::uint32_t VERSION_SIGNATURE_UNMARKED;
    static const boost::uint32_t VERSION_SIGNATURE_MARKED;
    static const boost::uint32_t DOC_SIGNATURE_START;
    static const boost::uint32_t DOC_SIGNATURE_END;

    class WriteFile {
    public:
        WriteFile(FILE *f);
        ~WriteFile();

        void write(const void *ptr, size_t size) const;

    private:
        FILE *f_;
    };

private:
    time_t min_time_;
    std::string root_;
    //boost::mutex mutexes_[256];
};

const time_t DocCacheDisk::DEFAULT_CACHE_TIME = 5; // sec
const boost::uint32_t DocCacheDisk::VERSION_SIGNATURE_UNMARKED = 0xdfc00201;
const boost::uint32_t DocCacheDisk::VERSION_SIGNATURE_MARKED = 0xdfc00202;
const boost::uint32_t DocCacheDisk::DOC_SIGNATURE_START = 0x0a0b0d0a;
const boost::uint32_t DocCacheDisk::DOC_SIGNATURE_END = 0x0a0e0d0a;

TaggedKeyDisk::TaggedKeyDisk(const Context *ctx, const CachedObject *obj) {
    assert(NULL != ctx);
    assert(NULL != obj);

    value_ = obj->createTagKey(ctx);

    std::string hash = HashUtils::hexMD5(value_.c_str(), value_.length());
    number_ = HashUtils::crc32(hash) & 0xFF;
    
    char buf[255];
    snprintf(buf, sizeof(buf), "%02x/%s", number_, hash.c_str());
    filename_.assign(buf);
}

boost::uint32_t
TaggedKeyDisk::number() const {
    return number_;
}

const std::string&
TaggedKeyDisk::asString() const {
    return value_;
}

const std::string&
TaggedKeyDisk::filename() const {
    return filename_;
}


DocCacheDisk::WriteFile::WriteFile(FILE *f) : f_(f) {
}

DocCacheDisk::WriteFile::~WriteFile() {
    if (f_) {
        fclose(f_);
    }
}

void DocCacheDisk::WriteFile::write(const void *ptr, size_t size) const {
    size_t sz = ::fwrite(ptr, 1, size, f_);
    if (sz != size) {
        char buf[60];
        snprintf(buf, sizeof(buf), "file write error size: %llu, written: %llu", (unsigned long long)size, (unsigned long long)sz);
        throw std::runtime_error(&buf[0]);
    }
}


DocCacheDisk::DocCacheDisk() : min_time_(DEFAULT_CACHE_TIME) {
    CacheStrategyCollector::instance()->addStrategy(this, name());
}

DocCacheDisk::~DocCacheDisk() {
}

void
DocCacheDisk::init(const Config *config) {
    DocCacheStrategy::init(config);

    Config::addForbiddenKey("/xscript/tagged-cache-disk/*");
    
    root_ = config->as<std::string>("/xscript/tagged-cache-disk/root-dir", "").append("/");
    min_time_ = config->as<time_t>("/xscript/tagged-cache-disk/min-cache-time", DEFAULT_CACHE_TIME);
    if (min_time_ <= 0) {
        min_time_ = DEFAULT_CACHE_TIME;
    }
    
    std::string no_cache =
        config->as<std::string>("/xscript/tagged-cache-disk/no-cache", StringUtils::EMPTY_STRING);

    insert2Cache(no_cache);
}

time_t
DocCacheDisk::minimalCacheTime() const {
    return min_time_;
}

std::string
DocCacheDisk::name() const {
    return "disk";
}

bool
DocCacheDisk::loadDocImpl(const TagKey *key, Tag &tag, XmlDocSharedHelper &doc, bool need_copy) {
    (void)need_copy;
    log()->debug("loading doc in disk cache");
    
    const TaggedKeyDisk *dkey = dynamic_cast<const TaggedKeyDisk*>(key);
    assert(NULL != dkey);

    std::string path(root_);
    path.append(dkey->filename());

    const std::string &key_str = key->asString();
    std::vector<char> vec;
    try {
        //boost::mutex::scoped_lock sl(mutexes_[dkey->number()]);
        if (!load(path, key_str, tag, vec)) {
            return false;
        }
    }
    catch (const std::exception &e) {
        log()->error("error while loading doc from cache: %s", e.what());
        return false;
    }

    try {
        doc = XmlDocSharedHelper(new XmlDocHelper(xmlParseMemory(&vec[0], vec.size())));
        XmlUtils::throwUnless(NULL != doc->get());
        return true;
    }
    catch (const std::exception &e) {
        log()->error("error while parsing doc from cache: %s", e.what());
        return false;
    }
}

bool
DocCacheDisk::saveDocImpl(const TagKey *key, const Tag &tag, const XmlDocSharedHelper &doc, bool need_copy) {
    (void)need_copy;
    log()->debug("saving doc in disk cache");
    
    const TaggedKeyDisk *dkey = dynamic_cast<const TaggedKeyDisk*>(key);
    assert(NULL != dkey);

    const std::string &key_str = key->asString();

    if (NULL == xmlDocGetRootElement(doc->get())) {
        log()->error("skip saving empty doc, key: %s", key_str.c_str());
        return false;
    }

    std::string path(root_);
    path.append(dkey->filename());

    try {
        createDir(path);

        char buf[path.length() + 8];
        buf[0] = 0;
        strcat(buf, path.c_str());
        strcat(buf, ".XXXXXX");
        int fd = mkstemp(buf);

        if (fd == -1) {
            log()->error("can not create filename: %s", buf);
            return false;
        }

        close(fd);

        if (!save(buf, key_str, tag, doc->get())) {
            log()->error("can not create doc: %s, key: %s", path.c_str(), key_str.c_str());
            return false;
        }

        if (rename(buf, path.c_str()) == 0) {
            return true;
        }

        log()->error("error while saving doc to cache: %d, key: %s", errno, key_str.c_str());
        return false;
    }
    catch (const std::exception &e) {
        log()->error("error while saving doc to cache: %s, key: %s", e.what(), key_str.c_str());
        return false;
    }
}

std::auto_ptr<TagKey>
DocCacheDisk::createKey(const Context *ctx, const CachedObject *obj) const {
    return std::auto_ptr<TagKey>(new TaggedKeyDisk(ctx, obj));
}

void
DocCacheDisk::makeDir(const std::string &name) {
    FileUtils::makeDir(name, 0777);
}

void
DocCacheDisk::createDir(const std::string &path) {
    std::string::size_type pos = 0;
    while (true) {
        pos = path.find('/', pos + 1);
        if (std::string::npos == pos) {
            break;
        }
        makeDir(path.substr(0, pos));
    }
}

bool
DocCacheDisk::load(const std::string &path, const std::string &key, Tag &tag, std::vector<char> &doc_data) {

    std::fstream is(path.c_str(), std::ios::in | std::ios::out);
    if (!is) {
        log()->debug("can not find cached doc");
        return false;
    }
    is.exceptions(std::ios::badbit | std::ios::eofbit);

    try {
        boost::uint32_t sig = 0, doclen = 0, key_size = 0;
        std::ifstream::pos_type size = 0;
        if (!is.seekg(0, std::ios::end)) {
            throw std::runtime_error("seek error");
        }
        size = is.tellg();
        if (!is.seekg(0, std::ios::beg)) {
            throw std::runtime_error("seek error");
        }

        is.read((char*) &sig, sizeof(boost::uint32_t));
        if (VERSION_SIGNATURE_MARKED != sig && VERSION_SIGNATURE_UNMARKED != sig) {
            throw std::runtime_error("bad signature");
        }
        is.read((char*) &tag.expire_time, sizeof(time_t));
        if (tag.expired()) {
            log()->info("tag expired");
            return false;
        }
        is.read((char*) &tag.last_modified, sizeof(time_t));

        time_t stored_time;
        is.read((char*) &stored_time, sizeof(time_t));

        if (VERSION_SIGNATURE_UNMARKED == sig && tag.needPrefetch(stored_time)) {
            log()->info("need prefetch doc");
            is.seekg(0, std::ios::beg);
            is.write((const char*) &VERSION_SIGNATURE_MARKED, sizeof(boost::uint32_t));
            return false;
        }

        is.read((char*) &key_size, sizeof(boost::uint32_t));

        std::string key_str;
        key_str.resize(key_size);
        is.read((char*) &key_str[0], key_size);
        if (key != key_str) {
            log()->info("tag key clashes with other one");
            return false;
        }

        is.read((char*) &sig, sizeof(boost::uint32_t));
        if (DOC_SIGNATURE_START != sig) {
            throw std::runtime_error("bad doc start signature");
        }
        size -= ((3 * sizeof(time_t)) + (3 * sizeof(boost::uint32_t)) + key_size);
        if (size < (std::ifstream::pos_type)(sizeof(boost::uint32_t))) {
            throw std::runtime_error("can not find doc end signature");
        }
        doclen = (boost::uint32_t)(size) - sizeof(boost::uint32_t);
        doc_data.resize(doclen);
        is.read(&doc_data[0], doclen);
        is.exceptions(std::ios::badbit);
        is.read((char*) &sig, sizeof(boost::uint32_t));
        if (DOC_SIGNATURE_END != sig) {
            throw std::runtime_error("bad doc end signature");
        }

        return true;
    }
    catch (const std::exception &e) {
        is.close();
        unlink(path.c_str());
        throw;
    }
}

bool
DocCacheDisk::save(const std::string &path, const std::string &key, const Tag &tag, xmlDocPtr doc) {

    log()->debug("saving %s, key: %s", path.c_str(), key.c_str());

    try {
        FILE *f = fopen(path.c_str(), "w");
        if (NULL == f) {
            return false;
        }
        WriteFile wf(f);
        wf.write(&VERSION_SIGNATURE_UNMARKED, sizeof(boost::uint32_t));
        wf.write(&tag.expire_time, sizeof(time_t));
        wf.write(&tag.last_modified, sizeof(time_t));

        const time_t now = time(NULL);
        wf.write(&now, sizeof(time_t));

        boost::uint32_t key_size = key.size();
        wf.write(&key_size, sizeof(boost::uint32_t));
        wf.write(key.data(), key_size);

        wf.write(&DOC_SIGNATURE_START, sizeof(boost::uint32_t));
        xmlDocDump(f, doc);
        wf.write(&DOC_SIGNATURE_END, sizeof(boost::uint32_t));

        return true;
    }
    catch (const std::exception &e) {
        unlink(path.c_str());
        throw;
    }
}

static ComponentRegisterer<DocCacheDisk> reg_;

} // namespace xscript
