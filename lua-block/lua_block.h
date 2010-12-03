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

private:
    virtual void postParse();
    virtual void property(const char *name, const char *value);

    void reportError(const char *message, lua_State *lua);
    virtual void call(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const throw (std::exception);

    void processEmptyLua(InvokeContext *invoke_ctx) const;
    void processLuaError(lua_State *lua) const;

private:
    const char* code_;
    std::string root_name_;
    xmlNsPtr root_ns_;
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
