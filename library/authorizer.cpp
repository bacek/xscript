#include "settings.h"

#include <cassert>

#include "xscript/request.h"
#include "xscript/context.h"
#include "xscript/response.h"
#include "xscript/authorizer.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

AuthContext::AuthContext() {
}

AuthContext::~AuthContext() {
}

bool
AuthContext::authorized() const {
	return true;
}

Authorizer::Authorizer()
{
}

Authorizer::~Authorizer() {
}

std::auto_ptr<AuthContext>
Authorizer::checkAuth(const boost::shared_ptr<Context> &ctx) const {
	(void)ctx;
	return std::auto_ptr<AuthContext>(new AuthContext());
}

void
Authorizer::redirectToAuth(const boost::shared_ptr<Context> &ctx, const AuthContext *auth) const {
	(void)ctx;
	(void)auth;
	ctx->response()->redirectToPath("/");
}

REGISTER_COMPONENT(Authorizer);

} // namespace xscript
