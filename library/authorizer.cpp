#include "settings.h"

#include <cassert>

#include "xscript/authorizer.h"
#include "xscript/context.h"
#include "xscript/request.h"
#include "xscript/response.h"
#include "xscript/string_utils.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

std::vector<std::string> Authorizer::bots_;

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
    return AuthContext::NOAUTH_STATUS;
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
Authorizer::checkAuth(Context *ctx) {
    (void)ctx;
    return boost::shared_ptr<AuthContext>(new AuthContext());
}

void
Authorizer::redirectToAuth(const Context *ctx, const AuthContext *auth) const {
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

static ComponentRegisterer<Authorizer> reg; 

} // namespace xscript
