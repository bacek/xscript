#include "settings.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "xscript/functors.h"
#include "xscript/guard_checker.h"

#ifndef HAVE_HASHMAP
#include <map>
#endif

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

typedef std::map<std::string, std::pair<GuardCheckerMethod, bool>, StringCILess> GuardCheckerMethodMap;

static GuardCheckerMethodMap methods;

GuardChecker::GuardChecker() {
}

GuardChecker::~GuardChecker() {
}

GuardChecker*
GuardChecker::instance() {
    static GuardChecker checker;
    return &checker;
}

void
GuardChecker::registerMethod(const char *type, GuardCheckerMethod method, bool allow_empty) {
    try {
        GuardCheckerMethodMap::iterator i = methods.find(type);
        if (methods.end() == i) {
            methods.insert(std::make_pair(type, std::make_pair(method, allow_empty)));
        }
        else {
            std::stringstream stream;
            stream << "duplicate param: " << type;
            throw std::invalid_argument(stream.str());
        }
    }
    catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        throw;
    }
}

GuardCheckerMethod
GuardChecker::method(const std::string &type) const {
    GuardCheckerMethodMap::const_iterator i = methods.find(type);
    if (methods.end() == i) {
        return NULL;
    }
    return i->second.first;
}

bool
GuardChecker::allowed(const char *type, bool is_empty) const {
    GuardCheckerMethodMap::const_iterator i = methods.find(type);
    if (methods.end() == i) {
        return false;
    }
    return is_empty ? i->second.second : true;
}

GuardCheckerRegisterer::GuardCheckerRegisterer(const char *type,
                                               GuardCheckerMethod method,
                                               bool allow_empty) {
    assert(type);
    GuardChecker::instance()->registerMethod(type, method, allow_empty);
}

} // namespace xscript
