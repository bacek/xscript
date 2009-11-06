#include "settings.h"

#include <cassert>

#include "xscript/authorizer.h"
#include "xscript/context.h"
#include "xscript/message_interface.h"
#include "xscript/request.h"
#include "xscript/response.h"
#include "xscript/string_utils.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

std::vector<std::string> Authorizer::bots_;

const std::string AuthContext::STATUS_METHOD = "AUTHCONTEXT_STATUS";
const std::string AuthContext::NOAUTH_STATUS = "noauth";

AuthContext::AuthContext() {
}

AuthContext::~AuthContext() {
}

bool
AuthContext::authorized() const {
    return true;
}

const std::string&
AuthContext::status() const {
    MessageParam<const AuthContext> auth_context_param(this);    
    MessageParamBase* param_list[1];
    param_list[0] = &auth_context_param;
    
    MessageParams params(1, param_list);
    MessageResult<const std::string*> result;
    
    MessageProcessor::instance()->process(STATUS_METHOD, params, result);
    return *result.get();
}

Authorizer::Authorizer() {
}

Authorizer::~Authorizer() {
}

void
Authorizer::init(const Config *config) {
    std::vector<std::string> v;
    Config::addForbiddenKey("/xscript/auth/bots/*");
    config->subKeys("/xscript/auth/bots/bot", v);
    for (std::vector<std::string>::iterator i = v.begin(), end = v.end(); i != end; ++i) {
        bots_.push_back(StringUtils::tolower(config->as<std::string>(*i)));
    }
}

boost::shared_ptr<AuthContext>
Authorizer::checkAuth(const boost::shared_ptr<Context> &ctx) {
    (void)ctx;
    return boost::shared_ptr<AuthContext>(new AuthContext());
}

void
Authorizer::redirectToAuth(const boost::shared_ptr<Context> &ctx, const AuthContext *auth) const {
    (void)ctx;
    (void)auth;
    ctx->response()->redirectToPath("/");
}

bool
Authorizer::isBot(const std::string &user_agent) {
    if (user_agent.empty() || bots_.empty()) {
        return false;
    }
    
    std::string cased_user_agent = StringUtils::tolower(user_agent);
    for(std::vector<std::string>::iterator it = bots_.begin();
        it != bots_.end();
        ++it) {
        if (std::string::npos != cased_user_agent.find(*it)) {
            return true;
        }
    }
    return false;
}

namespace AuthContextHandlers {

class StatusHandler : public MessageHandler {
    Result process(const MessageParams &params, MessageResultBase &result) {
        (void)params;
        result.set(&AuthContext::NOAUTH_STATUS);
        return BREAK;
    }
};


struct HandlerRegisterer {
    HandlerRegisterer() {
        MessageProcessor::instance()->registerBack(AuthContext::STATUS_METHOD,
                boost::shared_ptr<MessageHandler>(new StatusHandler()));
    }
};

static HandlerRegisterer reg_handlers;

} // namespace AuthContextHandlers

static ComponentRegisterer<Authorizer> reg; 

} // namespace xscript
