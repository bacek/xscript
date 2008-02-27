#include "settings.h"

#include <map>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <stdexcept>

#include "xscript/config.h"
#include "xscript/logger.h"
#include "xscript/object.h"
#include "xscript/context.h"
#include "standalone_request.h"
#include "server.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

void
parse(const char *argv, std::multimap<std::string, std::string> &m) {
	const char *pos = strchr(argv, '=');
	if (NULL != pos) {
		m.insert(std::pair<std::string, std::string>(std::string(argv, pos), std::string(pos + 1)));
	}
	else {
		m.insert(std::pair<std::string, std::string>(argv, "YES"));
	}
}

std::ostream&
processUsage(std::ostream &os) {
	os << "usage: xscript-proc --config=file file | url --header=<value> [--header=<value>] --dont-apply-stylesheet --dont-use-remote-call";
	return os;
}

int
main(int argc, char *argv[]) {
	
	using namespace xscript;
	try {
		std::auto_ptr<Config> config = Config::create(argc, argv, processUsage);

		std::string url;
		std::multimap<std::string, std::string> args;
		for (int i = 1; i <= argc; ++i) {
			if (strncmp(argv[i], "--", sizeof("--") - 1) == 0) {
				parse(argv[i] + sizeof("--") - 1, args);
			}
			else if (url.empty()) {
				url.assign(argv[i]);
			}
			else {
				throw std::runtime_error("url defined twice");
			}
		}
		if (args.end() != args.find("help")) {
			processUsage(std::cout) << std::endl;
			return EXIT_SUCCESS;
		}

		OfflineServer server(config.get(), url, args);
		server.run();

		return EXIT_SUCCESS;
	}
	catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}
