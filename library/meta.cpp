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

MetaCore::MetaCore() : elapsed_time_(undefinedElapsedTime())
{}

MetaCore::~MetaCore()
{}

void
MetaCore::reset() {
    data_.clear();
    elapsed_time_ = undefinedElapsedTime();
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
            elapsed_time_ = static_cast<int>(*((boost::int32_t*)key.begin()));
        }

        const char* buf_tmp = data.begin();
        const char* buf_end = data.end();
        while (buf_tmp < buf_end) {
            boost::uint32_t var = *((boost::uint32_t*)buf_tmp);
            buf_tmp += sizeof(boost::uint32_t);
            std::string key(buf_tmp, var);
            buf_tmp += var;

            var = *((boost::uint32_t*)buf_tmp);
            buf_tmp += sizeof(boost::uint32_t);
            unsigned int type = static_cast<unsigned int>(var);

            var = *((boost::uint32_t*)buf_tmp);
            buf_tmp += sizeof(boost::uint32_t);
            Range value(buf_tmp, buf_tmp + var);
            buf_tmp += var;

            if (!key.empty()) {
                data_.insert(key, TypedValue(type, value));
            }
        }
    }
    catch(const std::exception &e) {
        log()->error("exception caught while parsing block cache data: %s", e.what());
        return false;
    }
    return true;
}

void
MetaCore::serialize(std::string &buf) const {
    if (undefinedElapsedTime() != elapsed_time_) {
        buf.append("Elapsed-time:");
        boost::int32_t var = elapsed_time_;
        buf.append((char*)&var, sizeof(var));
        buf.append("\r\n");
    }
    const std::map<std::string, TypedValue>& values = data_.values();
    for (std::map<std::string, TypedValue>::const_iterator it = values.begin();
        it != values.end();
        ++it) {
        boost::uint32_t var = it->first.size();
        buf.append((char*)&var, sizeof(var));
        buf.append(it->first);
        var = it->second.type();
        buf.append((char*)&var, sizeof(var));
        std::string res;
        it->second.serialize(res);
        var = res.size();
        buf.append((char*)&var, sizeof(var));
        buf.append(res);
    }
}

int
MetaCore::undefinedElapsedTime() {
    return -1;
}

Meta::Meta() : expire_time_(undefinedExpireTime()), last_modified_(undefinedLastModified()),
        cache_params_writable_(false), core_writable_(true)
{}

Meta::~Meta()
{}

void
Meta::reset() {
    core_.reset();
    child_.clear();
    cache_params_writable_ = false;
    core_writable_ = true;
    expire_time_ = undefinedExpireTime();
    last_modified_ = undefinedLastModified();
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
    if (NULL == core_.get()) {
        core_ = boost::shared_ptr<MetaCore>(new MetaCore);
    }
}

void
Meta::setCore(const boost::shared_ptr<MetaCore> &core) {
    core_ = core;
}

boost::shared_ptr<MetaCore>
Meta::getCore() const {
    return core_;
}

std::string
Meta::get(const std::string &name, const std::string &default_value) const {
    TypedValue value;
    if (child_.find(name, value)) {
        return value.asString();
    }
    if (core_.get() && core_->data_.find(name, value)) {
        return value.asString();
    }
    return default_value;
}

bool
Meta::getTypedValue(const std::string &name, TypedValue &value) const {
    if (child_.find(name, value)) {
        return true;
    }
    if (core_.get() && core_->data_.find(name, value)) {
        return true;
    }
    return false;
}

void
Meta::setTypedValue(const std::string &name, const TypedValue &value) {
    if (!allowKey(name)) {
        throw std::runtime_error(name + " key is not allowed");
    }
    if (core_writable_) {
        initCore();
        core_->data_.insert(name, value);
    }
    else {
        child_.insert(name, value);
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
    setTypedValue(name, TypedValue(value));
}

void
Meta::setMap(const std::string &name, const std::map<std::string, std::string> &value) {
    setTypedValue(name, TypedValue(value));
}

bool
Meta::has(const std::string &name) const {
    if (child_.has(name)) {
        return true;
    }
    if (core_.get() && core_->data_.has(name)) {
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
    core_->elapsed_time_ = time;
}

int
Meta::getElapsedTime() const {
    return core_.get() ? core_->elapsed_time_ : MetaCore::undefinedElapsedTime();
}

time_t
Meta::getExpireTime() const {
    return expire_time_;
}

void
Meta::setExpireTime(time_t time) {
    if (cache_params_writable_) {
        expire_time_ = time;
    }
}

time_t
Meta::getLastModified() const {
    return last_modified_;
}

void
Meta::setLastModified(time_t time) {
    if (cache_params_writable_) {
        last_modified_ = time;
    }
}

void
Meta::setCacheParams(time_t expire, time_t last_modified) {
    expire_time_ = expire;
    last_modified_ = last_modified;
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

static void processNewMetaNode(const std::string &name, const TypedValue &value,
    XmlNodeHelper &root, xmlNodePtr &last_insert_node) {

    XmlTypedVisitor visitor(name);
    value.visit(&visitor, true);
    XmlNodeSetHelper res = visitor.result();
    while (res->nodeNr > 0) {
        if (last_insert_node) {
            xmlAddNextSibling(last_insert_node, res->nodeTab[0]);
            last_insert_node = res->nodeTab[0];
        }
        else {
            root = XmlNodeHelper(res->nodeTab[0]);
            last_insert_node = root.get();
        }
        xmlXPathNodeSetRemove(res.get(), 0);
    }
}

XmlNodeHelper
Meta::getXml() const {
    XmlNodeHelper result;
    xmlNodePtr last_insert_node = NULL;
    if (MetaCore::undefinedElapsedTime() != getElapsedTime()) {
        TypedValue value(boost::lexical_cast<std::string>(getElapsedTime()));
        processNewMetaNode(ELAPSED_TIME_META_KEY, value, result, last_insert_node);
    }

    if (undefinedExpireTime() != expire_time_) {
        TypedValue value(boost::lexical_cast<std::string>(expire_time_));
        processNewMetaNode(EXPIRE_TIME_META_KEY, value, result, last_insert_node);
    }

    if (undefinedExpireTime() != last_modified_) {
        TypedValue value(boost::lexical_cast<std::string>(last_modified_));
        processNewMetaNode(LAST_MODIFIED_META_KEY, value, result, last_insert_node);
    }

    const std::map<std::string, TypedValue> &values = child_.values();
    for(std::map<std::string, TypedValue>::const_iterator it = values.begin();
        it != values.end();
        ++it) {
        if (!allowKey(it->first)) {
            continue;
        }
        processNewMetaNode(it->first, it->second, result, last_insert_node);
    }

    if (NULL == core_.get()) {
        return result;
    }

    const std::map<std::string, TypedValue> &core_values = core_->data_.values();
    for(std::map<std::string, TypedValue>::const_iterator it = core_values.begin();
        it != core_values.end();
        ++it) {
        if (child_.has(it->first)) {
            continue;
        }
        processNewMetaNode(it->first, it->second, result, last_insert_node);
    }
    return result;
}

void
Meta::cacheParamsWritable(bool flag) {
    cache_params_writable_ = flag;
}

void
Meta::coreWritable(bool flag) {
    core_writable_ = flag;
}

} // namespace xscript
