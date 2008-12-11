#include "settings.h"

#include "standalone_request.h"
#include "offline_server.h"
#include "xslt_profiler.h"

#include "xscript/config.h"
#include "xscript/request_data.h"
#include "xscript/state.h"
#include "xscript/context.h"
#include "xscript/xml_util.h"

#include <boost/shared_ptr.hpp>
#include <boost/tokenizer.hpp>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

OfflineServer::OfflineServer(Config *config, const std::string &url, const std::multimap<std::string, std::string> &args) :
    Server(config), url_(url), apply_main_stylesheet_(true), apply_perblock_stylesheet_(true), use_remote_call_(true) {

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

        std::string xslt_path = config->as<std::string>("/xscript/offline/xslt-profile-path",
            "/usr/share/xscript-proc/profile.xsl");

        ComponentRegisterer<XsltProfiler> reg2(new OfflineXsltProfiler(xslt_path, text_mode));
        (void)reg2;
    }
}

OfflineServer::~OfflineServer() {
}

bool
OfflineServer::isOffline() const {
    return true;
}

void
OfflineServer::run() {

    XmlUtils::registerReporters();

    boost::shared_ptr<RequestResponse> request(new StandaloneRequest());
    StandaloneRequest* offline_request = dynamic_cast<StandaloneRequest*>(request.get());

    offline_request->attach(url_, root_);

    if (!use_remote_call_) {
        offline_request->setVariable("DONT_USE_REMOTE_CALL", "YES");
    }

    for (std::map<std::string, std::string>::iterator it_head = headers_.begin(); it_head != headers_.end(); ++it_head) {
        if (strncasecmp(it_head->first.c_str(), "Cookie", sizeof("Cookie") - 1) == 0) {

            typedef boost::char_separator<char> Separator;
            typedef boost::tokenizer<Separator> Tokenizer;

            Tokenizer tok(it_head->second, Separator("; "));
            for (Tokenizer::iterator it = tok.begin(), it_end = tok.end(); it != it_end; ++it) {
                std::string::size_type pos = it->find('=');
                if (std::string::npos != pos && pos < it->size() - 1) {
                    offline_request->addInputCookie(it->substr(0, pos), it->substr(pos + 1));
                }
            }
        }
        else {
            offline_request->addInputHeader(it_head->first, it_head->second);
        }
    }

    boost::shared_ptr<RequestData> data(
        new RequestData(request, boost::shared_ptr<State>(new State())));

    handleRequest(data);
}

bool
OfflineServer::needApplyMainStylesheet(Request *request) const {
    (void)request;
    return apply_main_stylesheet_;
}

bool
OfflineServer::needApplyPerblockStylesheet(Request *request) const {
    (void)request;
    return apply_perblock_stylesheet_;
}

Context*
OfflineServer::createContext(
    const boost::shared_ptr<Script> &script, const boost::shared_ptr<RequestData> &request_data) {
    std::auto_ptr<Context> ctx(new Context(script, request_data));
    ctx->xsltName(stylesheet_);
    return ctx.release();
}

}
