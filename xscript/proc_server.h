#ifndef _XSCRIPT_PROC_SERVER_H_
#define _XSCRIPT_PROC_SERVER_H_

#include "xscript/server.h"

namespace xscript {

class Config;

class ProcServer : public Server {
public:
    ProcServer(Config *config, const std::string &url, const std::multimap<std::string, std::string> &args);
    virtual ~ProcServer();
   
    virtual bool needApplyMainStylesheet(Request *request) const;
    virtual bool needApplyPerblockStylesheet(Request *request) const;
    virtual void run();
    virtual bool useXsltProfiler() const;

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

#endif // _XSCRIPT_PROC_SERVER_H_
