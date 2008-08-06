#ifndef _XSCRIPT_AUTHORIZER_H_
#define _XSCRIPT_AUTHORIZER_H_

#include <xscript/component.h>

namespace xscript {

class Context;

class AuthContext {
public:
    AuthContext();
    virtual ~AuthContext();
    virtual bool authorized() const;
};

class Authorizer : public virtual Component<Authorizer> {
public:
    Authorizer();
    virtual ~Authorizer();

    virtual boost::shared_ptr<AuthContext> checkAuth(const boost::shared_ptr<Context> &ctx) const;
    virtual void redirectToAuth(const boost::shared_ptr<Context> &ctx, const AuthContext *auth) const;
};

} // namespace xscript

#endif // _XSCRIPT_AUTHORIZER_H_
