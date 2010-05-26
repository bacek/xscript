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

Meta::Core::Core() : elapsed_time_(defaultElapsedTime())
{}

Meta::Core::~Core()
{}

void
Meta::Core::reset() {
    data_.clear();
    elapsed_time_ = defaultElapsedTime();
}

bool
Meta::Core::parse(const char *buf, boost::uint32_t size) {
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
            if (defaultElapsedTime() != elapsed_time_) {
                throw std::runtime_error("Elapsed-time specified twice");
            }
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
Meta::Core::serialize(std::string &buf) {
    buf.append("Elapsed-time:");
    buf.append(boost::lexical_cast<std::string>(elapsed_time_));
    buf.append("\r\n");
    for(MapType::iterator it = data_.begin();
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

Meta::Meta()
{}

Meta::~Meta()
{}

void
Meta::setCore(const boost::shared_ptr<Core> &core) {
    core_ = core;
}

boost::shared_ptr<Meta::Core>
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
    if (NULL == core_.get()) {
        core_ = boost::shared_ptr<Core>(new Core);
    }
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
    if (NULL == core_.get()) {
        core_ = boost::shared_ptr<Core>(new Core);
    }
    core_->elapsed_time_ = time;
}

int
Meta::defaultElapsedTime() {
    return -1;
}

int
Meta::getElapsedTime() const {
    return core_.get() ? core_->elapsed_time_ : defaultElapsedTime();
}

bool
Meta::allowKey(const std::string &key) const {
    if (ELAPSED_TIME_META_KEY == key) {
        return false;
    }
    return true;
}

XmlNodeHelper
Meta::getXml() const {
    XmlNodeHelper result(xmlNewNode(NULL, (const xmlChar*)ELAPSED_TIME_META_KEY.c_str()));
    xmlNodeSetContent(result.get(),
        (const xmlChar*)boost::lexical_cast<std::string>(getElapsedTime()).c_str());
    xmlNodePtr last_insert_node = result.get();
    for(MapType::const_iterator it = child_.begin();
        it != child_.end();
        ++it) {
        if (!allowKey(it->first)) {
            continue;
        }
        XmlNodeHelper node(xmlNewNode(NULL, (const xmlChar*)it->first.c_str()));
        xmlNodeSetContent(node.get(), (const xmlChar*)it->second.c_str());
        xmlAddNextSibling(last_insert_node, node.get());
        last_insert_node = node.release();
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
        XmlNodeHelper node(xmlNewNode(NULL, (const xmlChar*)it->first.c_str()));
        xmlNodeSetContent(node.get(), (const xmlChar*)it->second.c_str());
        xmlAddNextSibling(last_insert_node, node.get());
        last_insert_node = node.release();
    }
    return result;
}

} // namespace xscript
