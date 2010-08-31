#include "settings.h"

#include <stdexcept>

#include <boost/lexical_cast.hpp>

#include <libxml/xpathInternals.h>

#include "xscript/logger.h"
#include "xscript/meta.h"
#include "xscript/range.h"
#include "xscript/xml_util.h"

#include "internal/algorithm.h"
#include "internal/parser.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

static const std::string ELAPSED_TIME_META_KEY = "elapsed-time";
static const std::string EXPIRE_TIME_META_KEY = "expire-time";
static const std::string LAST_MODIFIED_META_KEY = "last-modified";

struct MetaCore::MetaCoreData {
    MetaCoreData() : elapsed_time_(MetaCore::undefinedElapsedTime())
    {}
    ~MetaCoreData()
    {}

    void reset() {
        data_.clear();
        elapsed_time_ = MetaCore::undefinedElapsedTime();
    }

    TypedMap data_;
    int elapsed_time_;
};

MetaCore::MetaCore() : core_data_(new MetaCoreData())
{}

MetaCore::~MetaCore()
{}

void
MetaCore::reset() {
    core_data_->reset();
}

static const Range ELAPSED_TIME_RANGE = createRange("Elapsed-time:");

bool
MetaCore::parse(const char *buf, boost::uint32_t size) {
    try {
        reset();
        Range data(buf, buf + size);

        if (startsWith(data, ELAPSED_TIME_RANGE)) {
            Range chunk;
            split(data, Parser::RN_RANGE, chunk, data);
            Range prefix, key;
            split(chunk, ':', prefix, key);
            if (key.empty()) {
                log()->error("incorrect elapsed time format in block cache data");
                return false;
            }
            core_data_->elapsed_time_ = static_cast<int>(*((boost::int32_t*)key.begin()));
        }

        const char* buf_tmp = data.begin();
        const char* buf_end = data.end();
        while (buf_tmp < buf_end) {
            if (buf_end - buf_tmp < (boost::int32_t)sizeof(boost::uint32_t)) {
                throw std::runtime_error("Incorrect meta format");
            }
            boost::uint32_t var = *((boost::uint32_t*)buf_tmp);
            buf_tmp += sizeof(boost::uint32_t);
            if (buf_end - buf_tmp < (boost::int64_t)var) {
                throw std::runtime_error("Incorrect meta format");
            }
            Range key(buf_tmp, buf_tmp + var);
            buf_tmp += var;
            if (buf_end - buf_tmp < (boost::int32_t)sizeof(boost::uint32_t)) {
                throw std::runtime_error("Incorrect meta format");
            }
            boost::uint32_t size = *((boost::uint32_t*)buf_tmp);
            if (buf_tmp + size > buf_end) {
                throw std::runtime_error("Incorrect meta format");

            }
            TypedValue value(Range(buf_tmp, buf_tmp + size));
            core_data_->data_.insert(std::string(key.begin(), key.end()), value);
            buf_tmp += size;
        }
    }
    catch (const std::exception &e) {
        log()->error("exception caught while parsing block cache data: %s", e.what());
        return false;
    }
    return true;
}

void
MetaCore::serialize(std::string &buf) const {
    if (undefinedElapsedTime() != core_data_->elapsed_time_) {
        buf.append("Elapsed-time:");
        boost::int32_t var = core_data_->elapsed_time_;
        buf.append((char*)&var, sizeof(var));
        buf.append("\r\n");
    }
    const std::map<std::string, TypedValue>& values = core_data_->data_.values();
    for (std::map<std::string, TypedValue>::const_iterator it = values.begin();
        it != values.end();
        ++it) {
        boost::uint32_t var = it->first.size();
        buf.append((char*)&var, sizeof(var));
        buf.append(it->first);
        it->second.serialize(buf);
    }
}

int
MetaCore::undefinedElapsedTime() {
    return -1;
}

const TypedValue&
MetaCore::find(const std::string &name) const {
    return core_data_->data_.findNoThrow(name);
}

void
MetaCore::insert(const std::string &name, const TypedValue &value) {
    core_data_->data_.insert(name, value);
}

bool
MetaCore::has(const std::string &name) const {
    return core_data_->data_.has(name);
}

time_t
MetaCore::getElapsedTime() const {
    return core_data_->elapsed_time_;
}

void
MetaCore::setElapsedTime(time_t time) {
    core_data_->elapsed_time_ = time;
}

const std::map<std::string, TypedValue>&
MetaCore::values() const {
    return core_data_->data_.values();
}

struct Meta::MetaData {
    MetaData() : expire_time_(Meta::undefinedExpireTime()),
            last_modified_(Meta::undefinedLastModified()),
            cache_params_writable_(false), core_writable_(true)
    {}
    ~MetaData()
    {}

    void reset() {
        core_.reset();
        child_.clear();
        cache_params_writable_ = false;
        core_writable_ = true;
        expire_time_ = Meta::undefinedExpireTime();
        last_modified_ = Meta::undefinedLastModified();
    }

    void initCore() {
        if (NULL == core_.get()) {
            core_ = boost::shared_ptr<MetaCore>(new MetaCore);
        }
    }

    boost::shared_ptr<MetaCore> core_;
    TypedMap child_;
    time_t expire_time_;
    time_t last_modified_;
    bool cache_params_writable_;
    bool core_writable_;
};

Meta::Meta() : meta_data_(new MetaData())
{}

Meta::~Meta()
{}

void
Meta::reset() {
    meta_data_->reset();
}

time_t
Meta::undefinedExpireTime() {
    return -1;
}

time_t
Meta::undefinedLastModified() {
    return -1;
}

void
Meta::initCore() {
    meta_data_->initCore();
}

void
Meta::setCore(const boost::shared_ptr<MetaCore> &core) {
    meta_data_->core_ = core;
}

boost::shared_ptr<MetaCore>
Meta::getCore() const {
    return meta_data_->core_;
}

const std::string&
Meta::get(const std::string &name, const std::string &default_value) const {
    const TypedValue& value = meta_data_->child_.findNoThrow(name);
    if (!value.nil()) {
        return value.asString();
    }
    if (meta_data_->core_.get()) {
        const TypedValue& value = meta_data_->core_->find(name);
        if (!value.nil()) {
            return value.asString();
        }
    }
    return default_value;
}

const TypedValue&
Meta::getTypedValue(const std::string &name) const {
    const TypedValue& value = meta_data_->child_.findNoThrow(name);
    if (!value.nil()) {
        return value;
    }
    return meta_data_->core_.get() ? meta_data_->core_->find(name) : value;
}

void
Meta::setTypedValue(const std::string &name, const TypedValue &value) {
    if (!allowKey(name)) {
        throw std::runtime_error(name + " key is not allowed");
    }
    if (meta_data_->core_writable_) {
        initCore();
        meta_data_->core_->insert(name, value);
    }
    else {
        meta_data_->child_.insert(name, value);
    }
}

void
Meta::setBool(const std::string &name, bool value) {
    setTypedValue(name, TypedValue(value));
}

void
Meta::setLong(const std::string &name, boost::int32_t value) {
    setTypedValue(name, TypedValue(value));
}

void
Meta::setULong(const std::string &name, boost::uint32_t value) {
    setTypedValue(name, TypedValue(value));
}

void
Meta::setLongLong(const std::string &name, boost::int64_t value) {
    setTypedValue(name, TypedValue(value));
}

void
Meta::setULongLong(const std::string &name, boost::uint64_t value) {
    setTypedValue(name, TypedValue(value));
}

void
Meta::setDouble(const std::string &name, double value) {
    setTypedValue(name, TypedValue(value));
}

void
Meta::setString(const std::string &name, const std::string &value) {
    setTypedValue(name, TypedValue(value));
}

void
Meta::setArray(const std::string &name, const std::vector<std::string> &value) {
    TypedValue val = TypedValue::createArrayValue();
    for (std::vector<std::string>::const_iterator it = value.begin();
         it != value.end();
         ++it) {
        val.add(StringUtils::EMPTY_STRING, TypedValue(*it));
    }
    setTypedValue(name, val);
}

void
Meta::setMap(const std::string &name, const std::vector<StringUtils::NamedValue> &value) {
    TypedValue val = TypedValue::createMapValue();
    for (std::vector<StringUtils::NamedValue>::const_iterator it = value.begin();
         it != value.end();
         ++it) {
        val.add(it->first, TypedValue(it->second));
    }
    setTypedValue(name, val);
}

bool
Meta::has(const std::string &name) const {
    if (meta_data_->child_.has(name)) {
        return true;
    }
    if (meta_data_->core_.get() && meta_data_->core_->has(name)) {
        return true;
    }
    return false;
}

void
Meta::setElapsedTime(int time) {
    if (time < 0) {
        throw std::runtime_error("Incorrect elapsed time value");
    }
    initCore();
    meta_data_->core_->setElapsedTime(time);
}

int
Meta::getElapsedTime() const {
    return meta_data_->core_.get() ?
        meta_data_->core_->getElapsedTime() :
            MetaCore::undefinedElapsedTime();
}

time_t
Meta::getExpireTime() const {
    return meta_data_->expire_time_;
}

void
Meta::setExpireTime(time_t time) {
    if (meta_data_->cache_params_writable_) {
        meta_data_->expire_time_ = time;
    }
}

time_t
Meta::getLastModified() const {
    return meta_data_->last_modified_;
}

void
Meta::setLastModified(time_t time) {
    if (meta_data_->cache_params_writable_) {
        meta_data_->last_modified_ = time;
    }
}

void
Meta::setCacheParams(time_t expire, time_t last_modified) {
    meta_data_->expire_time_ = expire;
    meta_data_->last_modified_ = last_modified;
}

bool
Meta::allowKey(const std::string &key) const {
    if (ELAPSED_TIME_META_KEY == key ||
        EXPIRE_TIME_META_KEY == key ||
        LAST_MODIFIED_META_KEY == key) {
        return false;
    }
    return true;
}

static void
processNewMetaNode(const std::string &name, const TypedValue &value,
        XmlNodeHelper &result, xmlNodePtr &last_node) {
    XmlTypedVisitor visitor;
    value.visit(&visitor);
    XmlNodeHelper res = visitor.result();
    xmlNewProp(res.get(), (const xmlChar*)"name", (const xmlChar*)name.c_str());
    if (result.get()) {
        last_node = xmlAddNextSibling(last_node, res.release());
    }
    else {
        result = res;
        last_node = result.get();
    }
}

XmlNodeHelper
Meta::getXml() const {
    XmlNodeHelper result;
    xmlNodePtr last_insert_node = NULL;
    if (MetaCore::undefinedElapsedTime() != getElapsedTime()) {
        TypedValue value((boost::int64_t)getElapsedTime());
        processNewMetaNode(ELAPSED_TIME_META_KEY, value, result, last_insert_node);
    }

    if (undefinedExpireTime() != meta_data_->expire_time_) {
        TypedValue value((boost::int64_t)meta_data_->expire_time_);
        processNewMetaNode(EXPIRE_TIME_META_KEY, value, result, last_insert_node);
    }

    if (undefinedExpireTime() != meta_data_->last_modified_) {
        TypedValue value((boost::int64_t)meta_data_->last_modified_);
        processNewMetaNode(LAST_MODIFIED_META_KEY, value, result, last_insert_node);
    }

    const std::map<std::string, TypedValue> &values = meta_data_->child_.values();
    for(std::map<std::string, TypedValue>::const_iterator it = values.begin();
        it != values.end();
        ++it) {
        if (!allowKey(it->first)) {
            continue;
        }
        processNewMetaNode(it->first, it->second, result, last_insert_node);
    }

    if (NULL == meta_data_->core_.get()) {
        return result;
    }

    const std::map<std::string, TypedValue> &core_values = meta_data_->core_->values();
    for(std::map<std::string, TypedValue>::const_iterator it = core_values.begin();
        it != core_values.end();
        ++it) {
        if (meta_data_->child_.has(it->first)) {
            continue;
        }
        processNewMetaNode(it->first, it->second, result, last_insert_node);
    }
    return result;
}

void
Meta::cacheParamsWritable(bool flag) {
    meta_data_->cache_params_writable_ = flag;
}

void
Meta::coreWritable(bool flag) {
    meta_data_->core_writable_ = flag;
}

} // namespace xscript
