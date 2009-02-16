#include <memory>
#include <string>

#include "xscript/config.h"
#include "xscript/string_utils.h"

#include "offline_server.h"
#include "server_wrapper.h"

namespace {
    std::auto_ptr<xscript::OfflineServer> server;
    std::auto_ptr<xscript::Config> config;
}

bool
initialize(const char *path) {
    try {
        server.reset(NULL);
        config = xscript::Config::create(path);
        server.reset(new xscript::OfflineServer(config.get()));
        return true;
    }
    catch (...) {
        return false;
    }
}


std::string
renderBuffer(const std::string &xml,
             const std::string &url,
             const std::string &docroot,
             const std::string &headers,
             const std::string &args) {
    try {
        if (!server.get()) {
            return xscript::StringUtils::EMPTY_STRING;
        }
        return server->renderBuffer(xml, url, docroot, headers, args);
    }
    catch (...) {
        return xscript::StringUtils::EMPTY_STRING;
    }
}


std::string
renderFile(const std::string &file,
           const std::string &docroot,
           const std::string &headers,
           const std::string &args) {
    try {
        if (!server.get()) {
            return xscript::StringUtils::EMPTY_STRING;
        }
        return server->renderFile(file, docroot, headers, args);
    }
    catch (...) {
        return xscript::StringUtils::EMPTY_STRING;
    }
}
