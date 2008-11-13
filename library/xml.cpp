#include "settings.h"

#include <stdexcept>
#include <boost/filesystem/path.hpp>

#include "xscript/policy.h"
#include "xscript/util.h"
#include "xscript/xml.h"
#include "xscript/xml_helpers.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

namespace fs = boost::filesystem;

Xml::Xml() {
}

Xml::~Xml() {
}

std::string
Xml::fullName(const std::string &object) const {

    if (object.empty()) {
        throw std::runtime_error("Empty relative path");
    }

    std::string res;
    if (*object.begin() == '/') {
        res.assign(object);
    }
    else {
        const std::string transformed = Policy::instance()->getPathByScheme(object);
        if (transformed.empty()) {
            throw std::runtime_error("Empty relative path");
        }
        if (*transformed.begin() == '/') {
            res.assign(transformed);
        }
        else {
            const std::string &owner_name = name();
            if (owner_name.empty()) {
                res.assign(transformed);
            }
            else if (*owner_name.rbegin() == '/') {
                res.assign(owner_name + transformed);
            }
            else {
                std::string::size_type pos = owner_name.find_last_of('/');
                if (pos == std::string::npos) {
                    res.assign(transformed);
                }
                else {
                    res.assign(owner_name.substr(0, pos + 1) + transformed);
                }
            }
        }
    }
#if BOOST_VERSION < 103401
    std::string::size_type length = res.length();
    for (std::string::size_type i = 0; i < length - 1; ++i) {
        if (res[i] == '/' && res[i + 1] == '/') {
            res.erase(i, 1);
            --i;
            --length;
        }
    }
#endif
    fs::path path = fs::path(res);
    path.normalize();
    return path.string();
}

void
Xml::swapModifiedInfo(TimeMapType &info) {
    modified_info_.swap(info);
}

const Xml::TimeMapType&
Xml::modifiedInfo() const {
    return modified_info_;
}


Xml::IncludeModifiedTimeSetter::IncludeModifiedTimeSetter(Xml *xml) : xml_(xml) {
    XmlInfoCollector::ready(true);
}

Xml::IncludeModifiedTimeSetter::~IncludeModifiedTimeSetter() {
    TimeMapType* modified_info = XmlInfoCollector::getModifiedInfo();
    if (NULL != modified_info) {
        xml_->swapModifiedInfo(*modified_info);
    }
    else {
        TimeMapType fake;
        xml_->swapModifiedInfo(fake);
    }
    XmlInfoCollector::ready(false);
}



} // namespace xscript
