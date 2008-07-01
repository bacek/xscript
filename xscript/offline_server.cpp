#include "settings.h"

#include "standalone_request.h"
#include "offline_server.h"

#include "xscript/checking_policy.h"
#include "xscript/config.h"
#include "xscript/request_data.h"
#include "xscript/state.h"
#include "xscript/util.h"

#include <boost/shared_ptr.hpp>
#include <boost/tokenizer.hpp>

#include <string>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

class OfflineCheckingPolicy : public DevelopmentCheckingPolicy
{
public:
	OfflineCheckingPolicy() {}
	virtual ~OfflineCheckingPolicy() {}
	bool useXSLTProfiler() const {return true;}
};

OfflineServer::OfflineServer(Config *config, const std::string& url, const std::multimap<std::string, std::string>& args) :
	Server(config), url_(url), apply_stylesheet_(true), use_remote_call_(true)
{
	ComponentRegisterer<CheckingPolicy> reg(new OfflineCheckingPolicy());
	(void)reg;

	root_ = config->as<std::string>("/xscript/offline/root-dir", "/usr/local/www");

	for (std::multimap<std::string, std::string>::const_iterator it = args.begin(); it != args.end(); ++it) {
		if (it->first == "header") {
			std::string::size_type pos = it->second.find(':');
			std::string::size_type pos_value = it->second.find_first_not_of(' ', pos + 1);

			if (std::string::npos != pos_value) {
				headers_.insert(std::make_pair(it->second.substr(0, pos), it->second.substr(pos_value)));
			}
		}
		else if (it->first == "dont-apply-stylesheet") {
			apply_stylesheet_ = false;
		}
		else if (it->first == "dont-use-remote-call") {
			use_remote_call_ = false;
		}
	}
}

OfflineServer::~OfflineServer() {
}

void
OfflineServer::run() {

	XmlUtils::registerReporters();

	StandaloneRequest request;
	request.attach(url_, root_);

	if (!use_remote_call_) {
		request.setVariable("DONT_USE_REMOTE_CALL", "YES");
	}

	for(std::map<std::string, std::string>::iterator it_head = headers_.begin(); it_head != headers_.end(); ++it_head) {
		if (strncasecmp(it_head->first.c_str(), "Cookie", sizeof("Cookie") - 1) == 0) {

			typedef boost::char_separator<char> Separator;
			typedef boost::tokenizer<Separator> Tokenizer;

			Tokenizer tok(it_head->second, Separator("; "));
			for(Tokenizer::iterator it = tok.begin(), it_end = tok.end(); it != it_end; ++it) {
				std::string::size_type pos = it->find('=');
				if (std::string::npos != pos && pos < it->size() - 1) {
					request.addInputCookie(it->substr(0, pos), it->substr(pos + 1));
				}
			}
		}
		else {
			request.addInputHeader(it_head->first, it_head->second);
		}
	}

	RequestData data(&request, &request, boost::shared_ptr<State>(new State()));
	handleRequest(&data);
}

bool
OfflineServer::needApplyStylesheet(Request *request) const {
	(void)request;
	return apply_stylesheet_;
}

}
