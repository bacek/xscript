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

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

void
parse(const char *argv, std::multimap<std::string, std::string> &m) {
	const char *pos = strchr(argv, '=');
	if (NULL != pos) {
		m.insert(std::pair<std::string, std::string>(std::string(argv, pos), std::string(pos + 1)));
	}
}

std::pair<std::string, std::string>
parseHeader(const std::string &param) {
	return std::pair<std::string, std::string>();
}

std::ostream&
processUsage(std::ostream &os) {
	os << "usage: xscript-proc --config=file file --mode=<verify-xml|verify-xsl> | url --header=<value> [--header=<value>]";
	return os;
}

void
processMode(const std::string &mode, const std::multimap<std::string, std::string> &args) {
	if ("verify-xml" == mode) {
	}
	else if ("verify-xml" == mode) {
	}
	else {
		std::stringstream stream;
		processUsage(stream);
		throw std::runtime_error(stream.str());
	}
}

int
main(int argc, char *argv[]) {
	
	using namespace xscript;
	try {
		
		std::auto_ptr<Config> config = Config::create(argc, argv, processUsage);
		config->startup();
		
		std::string url;
		std::multimap<std::string, std::string> args;
		for (int i = 1; i < argc; ++i) {
			if (strncmp(argv[i], "--", sizeof("--") - 1) == 0) {
				parse(argv[i] + sizeof("--") + 1, args);
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
		std::multimap<std::string, std::string>::iterator i = args.find("mode");
		if (args.end() != i) {
			processMode(i->second, args);
			return EXIT_SUCCESS;
		}
		return EXIT_SUCCESS;
	}
	catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}
