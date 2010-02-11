#ifndef _XSCRIPT_AUTHORIZER_H_
#define _XSCRIPT_AUTHORIZER_H_

#include <vector>

#include <xscript/component.h>

namespace xscript {

class Context;

class AuthContext {
public:
    AuthContext();
    virtual ~AuthContext();
    virtual bool authorized() const;
    const std::string& status() const;
    
    static const std::string STATUS_METHOD;
    static const std::string NOAUTH_STATUS;
};

class Authorizer : public virtual Component<Authorizer> {
public:
    Authorizer();
    virtual ~Authorizer();

    virtual void init(const Config *config);
    virtual boost::shared_ptr<AuthContext> checkAuth(Context *ctx);
    virtual void redirectToAuth(const Context *ctx, const AuthContext *auth) const;
    virtual bool isBot(const std::string &user_agent);
    
private:
    static std::vector<std::string> bots_;
};

} // namespace xscript

#endif // _XSCRIPT_AUTHORIZER_H_
