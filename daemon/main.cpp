#include "settings.h"

#include <locale>
#include <cstdlib>
#include <iostream>
#include <exception>

#include "server.h"
#include "xscript/config.h"
#include "xscript/logger.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

int
main(int argc, char *argv[]) {
	
	using namespace xscript;
	
	try {
		std::auto_ptr<Config> c = Config::create(argc, argv);
		Server server(c.get());
		server.run();
		
		return EXIT_SUCCESS;
	}
	catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}
