#ifndef _XSCRIPT_STANDALONE_REQUEST_H_
#define _XSCRIPT_STANDALONE_REQUEST_H_

#include <map>
#include <vector>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <boost/thread/mutex.hpp>

#include "xscript/functors.h"
#include "internal/request_impl.h"
#include "xscript/util.h"

#include "details/default_request_response.h"

namespace xscript {

class StandaloneRequest : public DefaultRequestResponse {
public:
    StandaloneRequest();
    virtual ~StandaloneRequest();

    virtual unsigned short getServerPort() const;
    virtual const std::string& getServerAddr() const;

    virtual const std::string& getPathInfo() const;

    virtual const std::string& getPathTranslated() const;

    virtual const std::string& getScriptName() const;
    virtual const std::string& getScriptFilename() const;
    virtual const std::string& getDocumentRoot() const;

    virtual const std::string& getQueryString() const;
    virtual const std::string& getRequestMethod() const;

    virtual bool isSecure() const;
    virtual std::streamsize write(const char *buf, std::streamsize size);

    virtual void sendError(unsigned short status, const std::string& message);
    virtual void attach(const std::string& url, const std::string& doc_root);

private:
    bool isSecure_;
    unsigned short port_;
    std::string host_, path_, script_name_, script_file_name_, query_, method_, root_directory_;
};

} // namespace xscript

#endif // _XSCRIPT_STANDALONE_REQUEST_H_
