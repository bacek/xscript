#include "settings.h"

#include <iostream>

#include "offline_request.h"
#include "proc_server.h"
#include "xslt_profiler.h"

#include "xscript/config.h"
#include "xscript/context.h"
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
    apply_main_stylesheet_(true), apply_perblock_stylesheet_(true), use_remote_call_(true) {

    root_ = config->as<std::string>("/xscript/offline/root-dir", "/usr/local/www");

    bool root_set_flag = false;
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
        else if (it->first == "profile" || it->first == "norman") {
            profile_it = it;
        }
        else if (it->first == "root-dir" || it->first == "docroot") {
            root_ = it->second;
            root_set_flag = true;
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

        std::string xslt_path = config->as<std::string>("/xscript/offline/xslt-profile-path",
            "/usr/share/xscript-proc/profile.xsl");

        ComponentRegisterer<XsltProfiler> reg2(new OfflineXsltProfiler(xslt_path, text_mode));
        (void)reg2;
    }
    
    if (!root_set_flag && !url_.empty() && url_[0] != '/' &&
         url_.find("://") == std::string::npos) {
        
        int max_path_size = 8192;
        char dirname[max_path_size];
        char *ret = getcwd(dirname, max_path_size);
        if (ret == NULL) {
            throw std::runtime_error("cannot read working directory");
        }

        root_ = std::string(dirname);
    }
}

ProcServer::~ProcServer() {
}

bool
ProcServer::isOffline() const {
    return true;
}

void
ProcServer::run() {

    XmlUtils::registerReporters();

    std::vector<std::string> args, headers;
    
    if (!use_remote_call_) {
        args.push_back("DONT_USE_REMOTE_CALL=1");
    }
    

    for (std::map<std::string, std::string>::iterator it_head = headers_.begin();
         it_head != headers_.end();
         ++it_head) {
        
        std::string header(it_head->first);
        header.push_back('=');
        header.append(it_head->second);        
        headers.push_back(header);
    }

    boost::shared_ptr<RequestResponse> request(new OfflineRequest());
    OfflineRequest* offline_request = dynamic_cast<OfflineRequest*>(request.get());
    
    bool attach_success = false;
    try {
        offline_request->attach(url_, root_, headers, args,
                StringUtils::EMPTY_STRING, &std::cout, &std::cerr);
        attach_success = true;
        
        boost::shared_ptr<RequestData> data(
            new RequestData(request, boost::shared_ptr<State>(new State())));

        handleRequest(data);
    }
    catch (const std::exception &e) {
        if (!attach_success) {
            OperationMode::instance()->sendError(offline_request, 400, e.what());
        }
        else {
            std::cout.flush();
            std::cerr.flush();
            throw;
        }
    }
    
    std::cout.flush();
    std::cerr.flush();
}

bool
ProcServer::needApplyMainStylesheet(Request *request) const {
    return apply_main_stylesheet_ ? Server::needApplyMainStylesheet(request) : apply_main_stylesheet_;
}

bool
ProcServer::needApplyPerblockStylesheet(Request *request) const {
    return apply_perblock_stylesheet_ ? Server::needApplyPerblockStylesheet(request) : apply_perblock_stylesheet_;
}

Context*
ProcServer::createContext(
    const boost::shared_ptr<Script> &script, const boost::shared_ptr<RequestData> &request_data) {
    std::auto_ptr<Context> ctx(new Context(script, request_data));
    ctx->xsltName(stylesheet_);
    return ctx.release();
}

}
