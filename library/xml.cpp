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

Xml::Xml() {
}

Xml::~Xml() {
}

std::string
Xml::filePath(const std::string &object) const {

    if (object.empty()) {
        throw std::runtime_error("Empty relative path");
    }
    if (*object.begin() == '/') {
        return object;
    }

    std::string transformed = Policy::instance()->getPathByScheme(NULL, object);
    if (transformed.empty()) {
        throw std::runtime_error("Empty relative path");
    }
    if (*transformed.begin() != '/') {
        const std::string &owner_name = name();
        if (!owner_name.empty()) {
            if (*owner_name.rbegin() == '/') {
                return owner_name + transformed;
            }

            std::string::size_type pos = owner_name.find_last_of('/');
            if (pos != std::string::npos) {
                return owner_name.substr(0, pos + 1) + transformed;
            }
        }
    }
    return transformed;
}


std::string
Xml::fullName(const std::string &name) const {
    return FileUtils::normalize(filePath(name));
}

void
Xml::swapModifiedInfo(TimeMapType &info) {
    modified_info_.swap(info);
}

const Xml::TimeMapType&
Xml::modifiedInfo() const {
    return modified_info_;
}

} // namespace xscript
