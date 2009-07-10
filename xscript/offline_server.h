#ifndef _XSCRIPT_OFFLINE_SERVER_H_
#define _XSCRIPT_OFFLINE_SERVER_H_

#include "xscript/server.h"

namespace xscript {

class Config;

class OfflineServer : public Server {
public:
    OfflineServer(Config *config);
    virtual ~OfflineServer();

    virtual std::string renderBuffer(const std::string &url,
                                     const std::string &xml,
                                     const std::string &body,
                                     const std::string &headers,
                                     const std::string &vars);
    
    virtual std::string renderFile(const std::string &file,
                                   const std::string &body,
                                   const std::string &headers,
                                   const std::string &vars);
    
    virtual bool useXsltProfiler() const;
    
protected:
    boost::shared_ptr<Script> getScript(Request *request);
    std::string root_;
};

} // namespace xscript

#endif // _XSCRIPT_OFFLINE_SERVER_H_
