#include "settings.h"

#include "offline_request.h"
#include "offline_server.h"
#include "xslt_profiler.h"

#include "xscript/config.h"
#include "xscript/context.h"
#include "xscript/operation_mode.h"
#include "xscript/request_data.h"
#include "xscript/script.h"
#include "xscript/state.h"
#include "xscript/xml_util.h"

#include <boost/shared_ptr.hpp>
#include <boost/tokenizer.hpp>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

OfflineServer::OfflineServer(Config *config) : Server(config) {
    root_ = config->as<std::string>("/xscript/offline/root-dir", "/usr/local/www");
}

OfflineServer::~OfflineServer() {
}

bool
OfflineServer::useXsltProfiler() const {
    return false;
}

std::string
OfflineServer::renderBuffer(const std::string &url,
                            const std::string &xml,
                            const std::string &body,
                            const std::string &headers,
                            const std::string &vars) {
    XmlUtils::registerReporters();

    std::vector<std::string> header_list, var_list;
    
    typedef boost::char_separator<char> Separator;
    typedef boost::tokenizer<Separator> Tokenizer;
    
    if (!headers.empty()) {
        Tokenizer tok(headers, Separator("\n\r"));
        for (Tokenizer::iterator it = tok.begin(), it_end = tok.end(); it != it_end; ++it) {
            header_list.push_back(*it);
        }
    }
    
    if (!vars.empty()) {
        Tokenizer tok(vars, Separator("\n\r"));
        for (Tokenizer::iterator it = tok.begin(), it_end = tok.end(); it != it_end; ++it) {
            var_list.push_back(*it);
        }
    }
    
    std::stringstream buffer;
    
    boost::shared_ptr<RequestResponse> request(new OfflineRequest(root_));
    OfflineRequest* offline_request = dynamic_cast<OfflineRequest*>(request.get());
    
    bool attach_success = false;
    try {
        offline_request->attach(url, xml, body, header_list, var_list, &buffer, &buffer, true);        
        attach_success = true;
        
        boost::shared_ptr<RequestData> data(
            new RequestData(request, boost::shared_ptr<State>(new State())));

        handleRequest(data);
    }
    catch (const std::exception &e) {
        if (!attach_success) {
            OperationMode::sendError(offline_request, 400, e.what());
        }
        else {
            OperationMode::sendError(offline_request, 500, e.what());
        }
    }
    
    return buffer.str();
}

std::string
OfflineServer::renderFile(const std::string &file,
                          const std::string &body,
                          const std::string &headers,
                          const std::string &vars) {
    return renderBuffer(file, StringUtils::EMPTY_STRING, body, headers, vars);
}

boost::shared_ptr<Script>
OfflineServer::getScript(Request *request) {
    
    OfflineRequest *offline_request = dynamic_cast<OfflineRequest*>(request);
    if (NULL == offline_request) {
        throw std::logic_error("Conflict: NULL or not a OfflineRequest in OfflineServer");
    }
    
    const std::string& xml = offline_request->xml();
    if (xml.empty()) {
        return Server::getScript(request);
    }
    
    return Script::create(request->getScriptFilename(), xml);
}

}
