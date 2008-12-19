#include "settings.h"

#include <cassert>

#include "xscript/request.h"
#include "xscript/context.h"
#include "xscript/response.h"
#include "xscript/authorizer.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

std::vector<std::string> Authorizer::bots_;

AuthContext::AuthContext() {
}

AuthContext::~AuthContext() {
}

bool
AuthContext::authorized() const {
    return true;
}

Authorizer::Authorizer() {
}

Authorizer::~Authorizer() {
}

void
Authorizer::init(const Config *config) {
    std::vector<std::string> v;
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

REGISTER_COMPONENT(Authorizer);

} // namespace xscript
