#include "settings.h"

#include <stdexcept>

#include "xscript/policy.h"
#include "xscript/util.h"
#include "xscript/xml.h"
#include "xscript/xml_helpers.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class Xml::XmlData {
public:
    XmlData(const std::string &name, Xml *owner);
    ~XmlData();
    
    std::string filePath(const std::string &object) const;
    
    Xml *owner_;
    std::string name_;
    TimeMapType modified_info_;
};

Xml::XmlData::XmlData(const std::string &name, Xml *owner) : owner_(owner), name_(name) {
}

Xml::XmlData::~XmlData() {
}

std::string
Xml::XmlData::filePath(const std::string &object) const {

    if (object.empty()) {
        throw std::runtime_error("Empty relative path");
    }
    if (*object.begin() == '/') {
        return object;
    }

    std::string transformed = Policy::getPathByScheme(NULL, object);
    if (transformed.empty()) {
        throw std::runtime_error("Empty relative path");
    }
    if (*transformed.begin() != '/') {
        if (!name_.empty()) {
            if (*name_.rbegin() == '/') {
                return name_ + transformed;
            }

            std::string::size_type pos = name_.find_last_of('/');
            if (pos != std::string::npos) {
                return name_.substr(0, pos + 1) + transformed;
            }
        }
    }
    return transformed;
}

Xml::Xml(const std::string &name) : data_(new XmlData(name, this)) {
}

Xml::~Xml() {
    delete data_;
}

const std::string&
Xml::name() const {
    return data_->name_;
}

std::string
Xml::fullName(const std::string &name) const {
    return FileUtils::normalize(data_->filePath(name));
}

void
Xml::swapModifiedInfo(TimeMapType &info) {
    data_->modified_info_.swap(info);
}

const Xml::TimeMapType&
Xml::modifiedInfo() const {
    return data_->modified_info_;
}

} // namespace xscript
