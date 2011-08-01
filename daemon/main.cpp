#include "settings.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <locale>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <sstream>

#include "fcgi_server.h"
#include "xscript/config.h"
#include "xscript/logger.h"
#include "xscript/string_utils.h"
#include "xscript/vhost_data.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

using namespace xscript;

bool
daemonize() {

    pid_t pid = fork();
    if (pid == -1) {
        std::stringstream ss;
        StringUtils::report("Could not become a daemon: fork #1 failed: ", errno, ss);
        throw std::logic_error(ss.str());
    }
    if (pid != 0) {
        _exit(0); // exit parent
    }

    pid_t sid = setsid();
    if (sid == -1) {
        std::stringstream ss;
        StringUtils::report("Could not become a daemon: setsid failed: ", errno, ss);
        throw std::logic_error(ss.str());
    }

    //check fork for child
    pid = fork();
    if (pid == -1) {
        std::stringstream ss;
        StringUtils::report("Could not become a daemon: fork #2 failed: ", errno, ss);
        throw std::logic_error(ss.str());
    }
    if (pid != 0) {
        _exit(0); // exit session leader
    }

    for (int i = getdtablesize(); i--; ) {
        close(i);
    }
    umask(0002); // disable: S_IWOTH
    chdir("/");

    const char *black_hole = "/dev/null";
    stdin = fopen(black_hole, "a+");
    if (stdin == NULL) {
        return false;
    }
    stdout = fopen(black_hole, "w");
    if (stdout == NULL) {
        return false;
    }
    stderr = fopen(black_hole, "w");
    if (stderr == NULL) {
        return false;
    }
    return true;
}

std::ostream&
processUsage(std::ostream &os) {
    os << "Usage: xscript-bin --config=<config file> [--daemon]";
    return os;
}

int
main(int argc, char *argv[]) {

    if (argc == 1) {
        processUsage(std::cerr) << std::endl;
        return EXIT_FAILURE;
    }

    bool cerr_disabled = false;
    try {
        for (int i = 1; i < argc; ++i) {
            if (!strcmp(argv[i], "--daemon")) {
                if (!daemonize()) {
                    return EXIT_FAILURE;
                }
                cerr_disabled = true;
                break;
            }
        }

        std::auto_ptr<Config> c = Config::create(argc, argv, false, &processUsage);
        VirtualHostData::instance()->setConfig(c.get());
        FCGIServer server(c.get());
        server.run();

        return EXIT_SUCCESS;
    }
    catch (const std::exception &e) {
        if (!cerr_disabled) {
            std::cerr << e.what() << std::endl;
        }
        else if (log() != NULL) {
            log()->crit("Exit on failure: %s\n", e.what());
        }
        return EXIT_FAILURE;
    }
}
