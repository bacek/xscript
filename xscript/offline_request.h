#ifndef _XSCRIPT_OFFLINE_REQUEST_H_
#define _XSCRIPT_OFFLINE_REQUEST_H_

#include <map>
#include <vector>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <boost/thread/mutex.hpp>

#include "xscript/functors.h"
#include "xscript/request_impl.h"
#include "xscript/util.h"

#include "internal/default_request_response.h"

namespace xscript {

class OfflineRequest : public DefaultRequestResponse {
public:
    OfflineRequest(const std::string &docroot);
    virtual ~OfflineRequest();

    virtual void attach(const std::string &uri,
                        const std::string &xml,
                        const std::string &body,
                        const std::vector<std::string> &headers,
                        const std::vector<std::string> &vars,
                        std::ostream *data_stream,
                        std::ostream *error_stream,
                        bool need_output);
    virtual bool suppressBody() const;
    const std::string& xml() const;

private:
    virtual void writeBuffer(const char *buf, std::streamsize size);
    virtual void writeError(unsigned short status, const std::string &message);
    virtual void writeByWriter(const BinaryWriter *writer);
    
    static void processHeaders(const std::vector<std::string> &headers,
                               unsigned long body_size,
                               std::vector<std::string> &env);
    void processVariables(const std::vector<std::string> &vars,
                          std::vector<std::string> &env);
    
private:
    std::ostream *data_stream_;
    std::ostream *error_stream_;
    std::string docroot_;
    std::string xml_;
    bool need_output_;
};

} // namespace xscript

#endif // _XSCRIPT_OFFLINE_REQUEST_H_
