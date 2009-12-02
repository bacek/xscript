#ifndef _XSCRIPT_LUA_BLOCK_H_
#define _XSCRIPT_LUA_BLOCK_H_

#include <lua.hpp>

#include "xscript/block.h"
#include "xscript/extension.h"

namespace xscript {

class State;
class Request;
class Response;

class LuaBlock : public Block {
public:
    LuaBlock(const Extension *ext, Xml *owner, xmlNodePtr node);
    virtual ~LuaBlock();

protected:
    virtual void postParse();

    void reportError(const char *message, lua_State *lua);
    virtual XmlDocHelper call(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) throw (std::exception);

private:
    const char *code_;
    static const std::string XSCRIPT_LUA;
};

class LuaExtension : public Extension {
public:
    LuaExtension();
    virtual ~LuaExtension();

    virtual const char* name() const;
    virtual const char* nsref() const;

    virtual void initContext(Context *ctx);
    virtual void stopContext(Context *ctx);
    virtual void destroyContext(Context *ctx);

    virtual std::auto_ptr<Block> createBlock(Xml *owner, xmlNodePtr node);
    virtual void init(const Config *config);
    
private:
    LuaExtension(const LuaExtension &);
    LuaExtension& operator = (const LuaExtension &);
};


} // namespace xscript

#endif // _XSCRIPT_LUA_BLOCK_H_
