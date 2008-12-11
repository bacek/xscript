#ifndef _XSCRIPT_OFFLINE_SERVER_H_
#define _XSCRIPT_OFFLINE_SERVER_H_

#include "xscript/server.h"

namespace xscript {

class Config;

class OfflineServer : public Server {
public:
    OfflineServer(Config *config, const std::string &url, const std::multimap<std::string, std::string> &args);
    virtual ~OfflineServer();

    virtual bool needApplyMainStylesheet(Request *request) const;
    virtual bool needApplyPerblockStylesheet(Request *request) const;
    virtual void run();
    virtual bool isOffline() const;

protected:
    virtual Context* createContext(
        const boost::shared_ptr<Script> &script, const boost::shared_ptr<RequestData> &request_data);

private:
    std::string root_;
    std::string url_;
    std::string stylesheet_;
    bool apply_main_stylesheet_, apply_perblock_stylesheet_, use_remote_call_;
    std::map<std::string, std::string> headers_;
};

} // namespace xscript

#endif // _XSCRIPT_OFFLINE_SERVER_H_
