#ifndef _XSCRIPT_OFFLINE_REQUEST_H_
#define _XSCRIPT_OFFLINE_REQUEST_H_

#include <map>
#include <vector>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <boost/thread/mutex.hpp>

#include "xscript/request.h"
#include "xscript/response.h"

namespace xscript {

class OfflineRequest : public Request {
public:
    OfflineRequest(const std::string &docroot);
    virtual ~OfflineRequest();

    void attach(const std::string &uri,
                const std::string &xml,
                const std::string &body,
                const std::vector<std::string> &headers,
                const std::vector<std::string> &vars);
    
    const std::string& xml() const;

private:   
    void processHeaders(const std::vector<std::string> &headers,
                        unsigned long body_size,
                        std::vector<std::string> &env);
    void processVariables(const std::vector<std::string> &vars,
                          std::vector<std::string> &env);
    
    void processPathInfo(const std::string &path,
                         std::vector<std::string> &env);
    
private:
    std::string docroot_;
    std::string xml_;
};

class OfflineResponse : public Response {
public:
    OfflineResponse(std::ostream *data_stream,
                    std::ostream *error_stream,
                    bool need_output);
    virtual ~OfflineResponse();

private:
    virtual bool suppressBody(const Request *req) const;
    virtual void writeError(unsigned short status, const std::string &message);
    virtual void writeHeaders();
    
private:
    std::ostream* error_stream_;
    bool need_output_;
};

} // namespace xscript

#endif // _XSCRIPT_OFFLINE_REQUEST_H_
