#include "settings.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

#include "xscript/guard_checker.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

GuardChecker::GuardChecker() {
}

GuardChecker::~GuardChecker() {
}

void
GuardChecker::registerMethod(const char *name, GuardCheckerMethod method) {
    try {
        GuardCheckerMethodMap::iterator i = methods_.find(name);
        if (methods_.end() == i) {
            methods_.insert(std::make_pair(name, method));
        }
        else {
            std::stringstream stream;
            stream << "duplicate param: " << name;
            throw std::invalid_argument(stream.str());
        }
    }
    catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        throw;
    }
}

GuardCheckerMethod
GuardChecker::method(const char *name) {
    GuardCheckerMethodMap::iterator i = methods_.find(name);
    if (methods_.end() == i) {
        return NULL;
    }
    return i->second;
}

GuardCheckerRegisterer::GuardCheckerRegisterer(const char *name, GuardCheckerMethod method) {
    assert(name);
    GuardChecker::instance()->registerMethod(name, method);
}

} // namespace xscript
