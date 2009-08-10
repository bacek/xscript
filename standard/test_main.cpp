#include "settings.h"

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


int
main(int argc, char *argv[]) {
    (void)argc;
    (void argv;

    try {
        CppUnit::TextUi::TestRunner r;
        r.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());
        return r.run("", false) ? EXIT_SUCCESS : EXIT_FAILURE;

    }
    catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
