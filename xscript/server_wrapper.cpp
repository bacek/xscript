#include <memory>
#include <string>

#include "xscript/config.h"
#include "xscript/memory_statistic.h"
#include "xscript/string_utils.h"

#include "offline_server.h"
#include "server_wrapper.h"

namespace {
    std::auto_ptr<xscript::OfflineServer> server;
    std::auto_ptr<xscript::Config> config;
}

bool
initialize(const char *config_path) {
    try {
        xscript::initAllocationStatistic();
        server.reset(NULL);
        config = xscript::Config::create(config_path);
        server.reset(new xscript::OfflineServer(config.get()));
        return true;
    }
    catch (...) {
        return false;
    }
}

std::string
renderBuffer(const std::string &url,
             const std::string &xml,
             const std::string &body,
             const std::string &headers,
             const std::string &vars) {
    try {
        if (!server.get()) {
            return xscript::StringUtils::EMPTY_STRING;
        }
        return server->renderBuffer(url, xml, body, headers, vars);
    }
    catch (...) {
        return xscript::StringUtils::EMPTY_STRING;
    }
}


std::string
renderFile(const std::string &file,
           const std::string &body,
           const std::string &headers,
           const std::string &vars) {
    try {
        if (!server.get()) {
            return xscript::StringUtils::EMPTY_STRING;
        }
        return server->renderFile(file, body, headers, vars);
    }
    catch (...) {
        return xscript::StringUtils::EMPTY_STRING;
    }
}
