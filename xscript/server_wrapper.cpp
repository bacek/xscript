#include <memory>
#include <stdexcept>
#include <string>

#include "xscript/config.h"
#include "xscript/string_utils.h"
#include "xscript/vhost_data.h"

#include "offline_server.h"
#include "server_wrapper.h"

namespace {
    std::auto_ptr<xscript::OfflineServer> server;
    std::auto_ptr<xscript::Config> config;
}

namespace xscript {
namespace offline {

void
initialize(const char *config_path) {
    if (server.get()) {
        throw std::runtime_error("server is already initialized");
    }
    config = xscript::Config::create(config_path);
    VirtualHostData::instance()->setConfig(config.get());
    server.reset(new xscript::OfflineServer(config.get()));
}

std::string
renderBuffer(const std::string &url,
             const std::string &xml,
             const std::string &body,
             const std::string &headers,
             const std::string &vars) {
    if (!server.get()) {
        throw std::runtime_error("server is not initialized");
    }
    return server->renderBuffer(url, xml, body, headers, vars);
}

std::string
renderFile(const std::string &file,
           const std::string &body,
           const std::string &headers,
           const std::string &vars) {
    if (!server.get()) {
        throw std::runtime_error("server is not initialized");
    }
    return server->renderFile(file, body, headers, vars);
}

}}
