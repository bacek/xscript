#ifndef _XSCRIPT_OFFLINE_SERVER_H_
#define _XSCRIPT_OFFLINE_SERVER_H_

#include "xscript/server.h"

namespace xscript {

class Config;

class OfflineServer : public Server {
public:
    OfflineServer(Config *config, const std::string& url, const std::multimap<std::string, std::string>& args);
    virtual ~OfflineServer();

    bool needApplyMainStylesheet(Request *request) const;
    bool needApplyPerblockStylesheet(Request *request) const;
    void run();
    bool isOffline() const;

private:
    std::string root_;
    std::string url_;
    bool apply_main_stylesheet_, apply_perblock_stylesheet_, use_remote_call_;
    std::map<std::string, std::string> headers_;
};

} // namespace xscript

#endif // _XSCRIPT_OFFLINE_SERVER_H_
