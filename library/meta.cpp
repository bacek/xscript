#include "settings.h"

#include <stdexcept>

#include <boost/lexical_cast.hpp>

#include "xscript/logger.h"
#include "xscript/meta.h"
#include "xscript/range.h"

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

bool
MetaCore::parse(const char *buf, boost::uint32_t size) {
    reset();
    Range data(buf, buf + size);
    while(!data.empty()) {
        Range chunk;
        split(data, Parser::RN_RANGE, chunk, data);
        if (chunk.empty()) {
            break;
        }

        Range prefix, key;
        split(chunk, ':', prefix, key);
        if (!prefix.empty() && strncmp(prefix.begin(), "Elapsed-time", sizeof("Elapsed-time") - 1) == 0) {
            elapsed_time_ = boost::lexical_cast<int>(std::string(key.begin(), key.size()));
            continue;
        }

        if (prefix.empty() || strncmp(prefix.begin(), "Key", sizeof("Key") - 1) != 0) {
            log()->error("incorrect key prefix while parsing block cache data");
            return false;
        }

        split(data, Parser::RN_RANGE, chunk, data);
        if (chunk.empty()) {
            log()->error("incorrect format while parsing block cache data");
            return false;
        }

        Range type;
        split(chunk, ':', prefix, type);
        if (prefix.empty() || strncmp(prefix.begin(), "Type", sizeof("Type") - 1) != 0) {
            log()->error("incorrect type prefix while parsing block cache data");
            return false;
        }

        if (type.empty() || strncmp(type.begin(), "string", sizeof("string") - 1) != 0) {
            log()->error("unknown meta type while parsing block cache data");
            return false;
        }

        split(data, Parser::RN_RANGE, chunk, data);
        if (chunk.empty()) {
            log()->error("incorrect format while parsing block cache data");
            return false;
        }

        Range value;
        split(chunk, ':', prefix, value);
        if (prefix.empty() || strncmp(prefix.begin(), "Value", sizeof("Value") - 1) != 0) {
            log()->error("incorrect value prefix while parsing block cache data");
            return false;
        }

        if (!key.empty()) {
            data_.insert(std::make_pair(
                std::string(key.begin(), key.size()),
                std::string(value.begin(), value.size())));
        }
    }
    return true;
}

void
MetaCore::serialize(std::string &buf) const {
    if (undefinedElapsedTime() != elapsed_time_) {
        buf.append("Elapsed-time:");
        buf.append(boost::lexical_cast<std::string>(elapsed_time_));
        buf.append("\r\n");
    }
    for(MapType::const_iterator it = data_.begin();
        it != data_.end();
        ++it) {
        buf.append("Key:");
        buf.append(it->first);
        buf.append("\r\n");
        buf.append("Type:");
        buf.append("string");
        buf.append("\r\n");
        buf.append("Value:");
        buf.append(it->second);
        buf.append("\r\n");
    }
}

int
MetaCore::undefinedElapsedTime() {
    return -1;
}

Meta::Meta() : expire_time_(undefinedExpireTime()), last_modified_(undefinedLastModified()),
        cache_params_writable_(false)
{}

Meta::~Meta()
{}

void
Meta::reset() {
    core_.reset();
    child_.clear();
    cache_params_writable_ = true;
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

const std::string&
Meta::get(const std::string &name, const std::string &default_value) const {
    MapType::const_iterator it = child_.find(name);
    if (child_.end() != it) {
        return it->second;
    }
    if (core_.get()) {
        it = core_->data_.find(name);
        if (core_->data_.end() != it) {
            return it->second;
        }
    }
    return default_value;
}

void
Meta::set(const std::string &name, const std::string &value) {
    child_[name] = value;
}

void
Meta::set2Core(const std::string &name, const std::string &value) {
    initCore();
    (core_->data_)[name] = value;
}

bool
Meta::has(const std::string &name) const {
    if (child_.end() != child_.find(name)) {
        return true;
    }
    if (core_.get() && core_->data_.end() != core_->data_.find(name)) {
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

bool
Meta::allowKey(const std::string &key) const {
    if (ELAPSED_TIME_META_KEY == key ||
        EXPIRE_TIME_META_KEY == key ||
        LAST_MODIFIED_META_KEY == key) {
        return false;
    }
    return true;
}

static void processNewMetaNode(const xmlChar *name, const xmlChar *value,
    XmlNodeHelper &root, xmlNodePtr &last_insert_node) {
    if (last_insert_node) {
        XmlNodeHelper node(xmlNewNode(NULL, name));
        xmlNodeSetContent(node.get(), value);
        xmlAddNextSibling(last_insert_node, node.get());
        last_insert_node = node.release();
    }
    else {
        root = XmlNodeHelper(xmlNewNode(NULL, name));
        xmlNodeSetContent(root.get(), value);
        last_insert_node = root.get();
    }
}

XmlNodeHelper
Meta::getXml() const {
    XmlNodeHelper result;
    xmlNodePtr last_insert_node = NULL;
    if (MetaCore::undefinedElapsedTime() != getElapsedTime()) {
        processNewMetaNode((const xmlChar*)ELAPSED_TIME_META_KEY.c_str(),
            (const xmlChar*)boost::lexical_cast<std::string>(getElapsedTime()).c_str(),
            result, last_insert_node);
    }

    if (undefinedExpireTime() != expire_time_) {
        processNewMetaNode((const xmlChar*)EXPIRE_TIME_META_KEY.c_str(),
            (const xmlChar*)boost::lexical_cast<std::string>(expire_time_).c_str(),
            result, last_insert_node);
    }

    if (undefinedLastModified() != last_modified_) {
        processNewMetaNode((const xmlChar*)LAST_MODIFIED_META_KEY.c_str(),
            (const xmlChar*)boost::lexical_cast<std::string>(last_modified_).c_str(),
            result, last_insert_node);
    }

    for(MapType::const_iterator it = child_.begin();
        it != child_.end();
        ++it) {
        if (!allowKey(it->first)) {
            continue;
        }
        processNewMetaNode((const xmlChar*)it->first.c_str(), (const xmlChar*)it->first.c_str(),
            result, last_insert_node);
    }

    if (NULL == core_.get()) {
        return result;
    }

    for(MapType::const_iterator it = core_->data_.begin();
        it != core_->data_.end();
        ++it) {
        if (!allowKey(it->first) || child_.end() != child_.find(it->first)) {
            continue;
        }
        processNewMetaNode((const xmlChar*)it->first.c_str(), (const xmlChar*)it->first.c_str(),
            result, last_insert_node);
    }
    return result;
}

void
Meta::cacheParamsWritable(bool flag) {
    cache_params_writable_ = flag;
}

} // namespace xscript
