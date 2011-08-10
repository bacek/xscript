#ifndef _XSCRIPT_LUA_BLOCK_H_
#define _XSCRIPT_LUA_BLOCK_H_

#include <boost/function.hpp>

#include <lua.hpp>

#include "xscript/renamed_block.h"
#include "xscript/extension.h"

namespace xscript {

class LuaExtension;
class State;
class Request;
class Response;

class LuaBlock : public RenamedBlock {
public:
    LuaBlock(const LuaExtension *ext, Xml *owner, xmlNodePtr node);
    virtual ~LuaBlock();

    const std::string& getBase() const;

private:
    static const char* findCode(xmlNodePtr node);

    virtual void postParse();

    void reportError(const char *message, lua_State *lua);
    virtual void call(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const throw (std::exception);

    void processEmptyLua(InvokeContext *invoke_ctx) const;
    void processLuaError(lua_State *lua) const;

private:
    const LuaExtension* ext_;
    const char* code_;
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
    
    static void registerLib(lua_State *lua, const char *name, bool builtin,
            const struct luaL_reg *lib_methods, const struct luaL_reg *lib_funcs);

    void registerExtensions(lua_State *lua) const;

    typedef boost::function<void (lua_State*)> LuaRegisterFunc;
private:
    LuaExtension(const LuaExtension &);
    LuaExtension& operator = (const LuaExtension &);

private:
    std::vector<LuaRegisterFunc> funcs_;
};

} // namespace xscript

#endif // _XSCRIPT_LUA_BLOCK_H_
