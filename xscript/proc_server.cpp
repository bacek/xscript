#include "settings.h"

#include <iostream>

#include "offline_request.h"
#include "proc_server.h"
#include "xslt_profiler.h"

#include "xscript/config.h"
#include "xscript/context.h"
#include "xscript/exception.h"
#include "xscript/operation_mode.h"
#include "xscript/request_data.h"
#include "xscript/state.h"
#include "xscript/xml_util.h"

#include <boost/shared_ptr.hpp>
#include <boost/tokenizer.hpp>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

ProcServer::ProcServer(Config *config,
                       const std::string &url,
                       const std::multimap<std::string, std::string> &args) :
    Server(config), url_(url),
    apply_main_stylesheet_(true), apply_perblock_stylesheet_(true),
    use_remote_call_(true), need_output_(true), xslt_profiler_(false) {

    root_ = config->as<std::string>("/xscript/offline/root-dir", "/usr/local/www");
    
    std::multimap<std::string, std::string>::const_iterator profile_it = args.end();
    for (std::multimap<std::string, std::string>::const_iterator it = args.begin(); it != args.end(); ++it) {
        if (it->first == "header") {
            std::string::size_type pos = it->second.find(':');
            std::string::size_type pos_value = it->second.find_first_not_of(' ', pos + 1);

            if (std::string::npos != pos_value) {
                headers_.insert(std::make_pair(it->second.substr(0, pos), it->second.substr(pos_value)));
            }
        }
        else if (it->first == "dont-apply-stylesheet") {
            apply_main_stylesheet_ = false;
            if (it->second == "all") {
                apply_perblock_stylesheet_ = false;
            }
        }
        else if (it->first == "dont-use-remote-call") {
            use_remote_call_ = false;
        }
        else if (it->first == "noout") {
            need_output_ = false;
        }
        else if (it->first == "profile" || it->first == "norman") {
            profile_it = it;
        }
        else if (it->first == "root-dir" || it->first == "docroot") {
            root_ = it->second;
        }
        else if (it->first == "stylesheet") {
            stylesheet_ = it->second;
        }
    }

    if (profile_it != args.end()) {
        bool text_mode = true;
        if (profile_it->second.empty() || profile_it->second == "text") {
        }
        else if (profile_it->second == "xml") {
            text_mode = false;
        }
        else {
            throw std::runtime_error(std::string("Unknown value of profile argument: ") + profile_it->second);
        }

        std::string xslt_path;
        if (text_mode) {
            xslt_path = config->as<std::string>("/xscript/offline/xslt-profile-path",
                    "/usr/share/xscript-proc/profile.xsl");
        }

        ComponentImplRegisterer<XsltProfiler> reg2(new OfflineXsltProfiler(xslt_path));
        (void)reg2;
        xslt_profiler_ = true;
    }
    
    if (!url_.empty() && url_[0] != '/' &&
         url_.find("://") == std::string::npos) {
        
        int max_path_size = 8192;
        char dirname[max_path_size];
        char *ret = getcwd(dirname, max_path_size);
        if (ret == NULL) {
            throw std::runtime_error("cannot read working directory");
        }

        root_ = std::string(dirname);
    }
    
    config->stopCollectCache();
}

ProcServer::~ProcServer() {
}

bool
ProcServer::useXsltProfiler() const {
    return xslt_profiler_;
}

void
ProcServer::run() {

    XmlUtils::registerReporters();

    std::vector<std::string> vars, headers;
    
    if (!use_remote_call_) {
        vars.push_back("DONT_USE_REMOTE_CALL=1");
    }
    

    for (std::map<std::string, std::string>::iterator it_head = headers_.begin();
         it_head != headers_.end();
         ++it_head) {
        
        std::string header(it_head->first);
        header.push_back('=');
        header.append(it_head->second);        
        headers.push_back(header);
    }

    boost::shared_ptr<Context> ctx;
    boost::shared_ptr<Request> request(new OfflineRequest(root_));
    boost::shared_ptr<Response> response(new OfflineResponse(&std::cout, &std::cerr, need_output_));
    ResponseDetacher response_detacher(response.get(), ctx);
    
    OfflineRequest* offline_request = dynamic_cast<OfflineRequest*>(request.get());

    try {        
        offline_request->attach(url_,
                                StringUtils::EMPTY_STRING,
                                StringUtils::EMPTY_STRING,
                                headers,
                                vars);

        
        handleRequest(request, response, ctx);
    }
    catch (const BadRequestError &e) {
        OperationMode::instance()->sendError(response.get(), 400, e.what());
    }
    catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    
    std::cout.flush();
    std::cerr.flush();
}

bool
ProcServer::needApplyMainStylesheet(Request *request) const {
    return apply_main_stylesheet_ ?
        Server::needApplyMainStylesheet(request) : apply_main_stylesheet_;
}

bool
ProcServer::needApplyPerblockStylesheet(Request *request) const {
    return apply_perblock_stylesheet_ ?
        Server::needApplyPerblockStylesheet(request) : apply_perblock_stylesheet_;
}

Context*
ProcServer::createContext(const boost::shared_ptr<Script> &script,
                          const boost::shared_ptr<State> &state,
                          const boost::shared_ptr<Request> &request,
                          const boost::shared_ptr<Response> &response) {
    std::auto_ptr<Context> ctx(Server::createContext(script, state, request, response));
    if (!stylesheet_.empty()) {
        ctx->xsltName(stylesheet_);
    }
    return ctx.release();
}

}
