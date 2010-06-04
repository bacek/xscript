#include "settings.h"

#include <sstream>

#include "xscript/exception.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

InvokeError::InvokeError(const std::string &error) : UnboundRuntimeError(error)
{}

InvokeError::InvokeError(const std::string &error, XmlNodeHelper node) :
    UnboundRuntimeError(error), node_(node)
{}

InvokeError::InvokeError(const std::string &error, const std::string &name, const std::string &value) :
    UnboundRuntimeError(error) {
    addEscaped(name, value);
}

InvokeError::InvokeError(const std::string &error, const InfoMapType &info) :
        UnboundRuntimeError(error), info_(info)
{}

void
InvokeError::add(const std::string &name, const std::string &value) {
    if (name.empty()) {
        throw std::runtime_error("empty key");
    }
    if (!value.empty()) {
        std::pair<std::string, std::string> info = std::make_pair(name, value);
        if (info_.empty()) {
            info_.push_back(info);
            return;
        }
        InfoMapType::iterator it = std::find(info_.begin(), info_.end(), info);
        if (info_.end() == it) {
            info_.push_back(info);
        }
    }
}

void
InvokeError::addEscaped(const std::string &name, const std::string &value) {
    add(name, XmlUtils::escape(value));
}

std::string
InvokeError::what_info() const throw() {

    std::stringstream stream;
    stream << what() << ". ";

    for(InvokeError::InfoMapType::const_iterator it = info_.begin();
        it != info_.end();
        ++it) {
        stream << it->first << ": " << it->second << ". ";
    }

    return stream.str();
}

} // namespace xscript
