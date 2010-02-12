#include "settings.h"

#include <boost/current_function.hpp>

#include "xscript/context.h"
#include "xscript/logger.h"

#include "exception.h"
#include "local_methods.h"
#include "stack.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

extern "C" {
    int luaLocalHas(lua_State *lua);
    int luaLocalGet(lua_State *lua);
    int luaLocalIs(lua_State *lua);
}

static const struct luaL_reg locallib [] = {
    {"has",             luaLocalHas},
    {"get",             luaLocalGet},
    {"is",              luaLocalIs},
    {NULL, NULL}
};

const struct luaL_reg * getLocalLib() {
    return locallib;
}

extern "C" {

int
luaLocalHas(lua_State *lua) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 2);

        Context *ctx = luaReadStack<Context>(lua, "xscript.localargs", 1);
        std::string key = luaReadStack<std::string>(lua, 2);
        log()->debug("luaLocalHas: %s", key.c_str());
        lua_pushboolean(lua, ctx->hasLocalParam(key));
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        luaL_error(lua, "caught exception in localargs.has: %s", e.what());
        return 0;
    }
}

int
luaLocalGet(lua_State *lua) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        int count = lua_gettop(lua);
        if (count < 2 || count > 3) {
            throw BadArgCount(count);
        }
        Context* ctx = luaReadStack<Context>(lua, "xscript.localargs", 1);
        std::string key = luaReadStack<std::string>(lua, 2);
        log()->debug("luaLocalGet: %s", key.c_str());
        std::string def_value;
        if (3 == count) {
            def_value = luaReadStack<std::string>(lua, 3);
        }
        lua_pushstring(lua, ctx->getLocalParam(key, def_value).c_str());
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        luaL_error(lua, "caught exception in localargs.get: %s", e.what());
        return 0;
    }
}

int
luaLocalIs(lua_State *lua) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 2);

        Context* ctx = luaReadStack<Context>(lua, "xscript.localargs", 1);
        std::string key = luaReadStack<std::string>(lua, 2);
        log()->debug("luaLocalIs: %s", key.c_str());
        lua_pushboolean(lua, ctx->localParamIs(key));
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        luaL_error(lua, "caught exception in localargs.is: %s", e.what());
        return 0;
    }
}

} // extern "C"

} // namespace xscript

