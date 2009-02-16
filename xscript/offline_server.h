#ifndef _XSCRIPT_OFFLINE_SERVER_H_
#define _XSCRIPT_OFFLINE_SERVER_H_

#include "xscript/server.h"

namespace xscript {

class Config;

class OfflineServer : public Server {
public:
    OfflineServer(Config *config);
    virtual ~OfflineServer();

    virtual std::string renderBuffer(const std::string &xml,
                                     const std::string &url,
                                     const std::string &docroot,
                                     const std::string &headers,
                                     const std::string &args);
    
    virtual std::string renderFile(const std::string &file,
                                   const std::string &docroot,
                                   const std::string &headers,
                                   const std::string &args);
    
    virtual bool isOffline() const;
    
protected:
    boost::shared_ptr<Script> getScript(const std::string &script_name, Request *request);
};

} // namespace xscript

#endif // _XSCRIPT_OFFLINE_SERVER_H_
