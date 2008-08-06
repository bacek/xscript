#include "settings.h"

#include <cstring>
#include <map>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <boost/lexical_cast.hpp>

#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include "xscript/config.h"
#include "xscript/logger.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

extern "C" void*
    xmlCustomMalloc(size_t size) {
    void *ptr = malloc(size);
    log()->debug("malloc'd %llu bytes at %p", static_cast<unsigned long long>(size), ptr);
    return ptr;
}

extern "C" void*
    xmlCustomRealloc(void *ptr, size_t size) {
    void *newptr = realloc(ptr, size);
    log()->debug("realloc'd %llu bytes at %p", static_cast<unsigned long long>(size), newptr);
    return newptr;
}

extern "C" void
    xmlCustomFree(void *ptr) {
    free(ptr);
}

extern "C" char*
    xmlCustomStrdup(const char *str) {
    char *ptr = strdup(str);
    log()->debug("strdup'd at %p", ptr);
    return ptr;
}

} // namespace xscript

int
main(int argc, char *argv[]) {

    using namespace xscript;

    try {
        std::auto_ptr<Config> config =  Config::create("test.conf");
        config->startup();

        CppUnit::TextUi::TestRunner r;
        r.addTest(CppUnit::TestFactoryRegistry::getRegistry("load").makeTest());
        r.addTest(CppUnit::TestFactoryRegistry::getRegistry("xscript").makeTest());
        return r.run((argc > 1) ? argv[1] : "", false) ? EXIT_SUCCESS : EXIT_FAILURE;

    }
    catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
