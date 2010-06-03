#include "settings.h"

#include <boost/current_function.hpp>

#include "xscript/block.h"
#include "xscript/context.h"
#include "xscript/logger.h"
#include "xscript/meta.h"

#include "exception.h"
#include "meta_methods.h"
#include "stack.h"
#include "xscript_methods.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

extern "C" {
    int luaMetaHas(lua_State *lua);
    int luaMetaGet(lua_State *lua);
    int luaMetaSet(lua_State *lua);
    int luaMetaGetElapsedTime(lua_State *lua);
    int luaMetaGetExpireTime(lua_State *lua);
    int luaMetaSetExpireTime(lua_State *lua);
    int luaMetaGetLastModified(lua_State *lua);
    int luaMetaSetLastModified(lua_State *lua);

    int luaLocalMetaHas(lua_State *lua);
    int luaLocalMetaGet(lua_State *lua);
    int luaLocalMetaSet(lua_State *lua);
}

static const struct luaL_reg metalib [] = {
    {"has",             luaMetaHas},
    {"get",             luaMetaGet},
    {"set",             luaMetaSet},
    {"getElapsedTime",  luaMetaGetElapsedTime},
    {"getExpireTime",   luaMetaGetExpireTime},
    {"setExpireTime",   luaMetaSetExpireTime},
    {"getLastModified", luaMetaGetLastModified},
    {"setLastModified", luaMetaSetLastModified},
    {NULL, NULL}
};

const struct luaL_reg * getMetaLib() {
    return metalib;
}

static const struct luaL_reg localmetalib [] = {
    {"has",             luaLocalMetaHas},
    {"get",             luaLocalMetaGet},
    {"set",             luaLocalMetaSet},
    {NULL, NULL}
};

const struct luaL_reg * getLocalMetaLib() {
    return localmetalib;
}

extern "C" {

static void
readMetaFromStack(lua_State *lua, bool local) {
    const char *name = local ? "xscript.localmeta" : "xscript.meta";
    luaReadStack<void>(lua, name, 1);
}

static InvokeContext*
metaInvokeContext(lua_State *lua, bool local) {
    InvokeContext* invoke_ctx = getInvokeContext(lua);
    if (local) {
        return invoke_ctx;
    }
    return invoke_ctx->parent(getContext(lua));
}

static int
luaMetaHasInternal(lua_State *lua, bool local) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 2);
        readMetaFromStack(lua, local);
        InvokeContext* invoke_ctx = metaInvokeContext(lua, local);
        std::string key = luaReadStack<std::string>(lua, 2);
        lua_pushboolean(lua, invoke_ctx ? invoke_ctx->meta()->has(key) : false);
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        luaL_error(lua, "caught exception in meta.has: %s", e.what());
        return 0;
    }
}

static int
luaMetaGetInternal(lua_State *lua, bool local) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        int count = lua_gettop(lua);
        if (count < 2 || count > 3) {
            throw BadArgCount(count);
        }
        readMetaFromStack(lua, local);
        InvokeContext* invoke_ctx = metaInvokeContext(lua, local);
        std::string key = luaReadStack<std::string>(lua, 2);
        std::string def_value;
        if (3 == count) {
            def_value = luaReadStack<std::string>(lua, 3);
        }
        lua_pushstring(lua, invoke_ctx ?
            invoke_ctx->meta()->get(key, def_value).c_str() : def_value.c_str());
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        luaL_error(lua, "caught exception in meta.get: %s", e.what());
        return 0;
    }
}

static int
luaMetaSetInternal(lua_State *lua, bool local) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 3);
        readMetaFromStack(lua, local);
        InvokeContext* invoke_ctx = metaInvokeContext(lua, local);
        std::string key = luaReadStack<std::string>(lua, 2);
        std::string value = luaReadStack<std::string>(lua, 3);
        if (invoke_ctx) {
            invoke_ctx->meta()->set(key, value);
        }
        luaPushStack(lua, value);
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        luaL_error(lua, "caught exception in meta.set: %s", e.what());
        return 0;
    }
}

int
luaMetaHas(lua_State *lua) {
    return luaMetaHasInternal(lua, false);
}

int
luaLocalMetaHas(lua_State *lua) {
    return luaMetaHasInternal(lua, true);
}

int
luaMetaGet(lua_State *lua) {
    return luaMetaGetInternal(lua, false);
}

int
luaLocalMetaGet(lua_State *lua) {
    return luaMetaGetInternal(lua, true);
}

int
luaMetaSet(lua_State *lua) {
    return luaMetaSetInternal(lua, false);
}

int
luaLocalMetaSet(lua_State *lua) {
    return luaMetaSetInternal(lua, true);
}

int
luaMetaGetElapsedTime(lua_State *lua) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 1);
        readMetaFromStack(lua, false);
        InvokeContext* invoke_ctx = metaInvokeContext(lua, false);
        lua_pushnumber(lua, invoke_ctx ?
            invoke_ctx->meta()->getElapsedTime() : MetaCore::undefinedElapsedTime());
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        luaL_error(lua, "caught exception in meta.getElapsedTime: %s", e.what());
        return 0;
    }
}

int
luaMetaGetExpireTime(lua_State *lua) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 1);
        readMetaFromStack(lua, false);
        InvokeContext* invoke_ctx = metaInvokeContext(lua, false);
        lua_pushnumber(lua, invoke_ctx ?
            invoke_ctx->meta()->getExpireTime() : Meta::undefinedExpireTime());
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        luaL_error(lua, "caught exception in meta.getElapsedTime: %s", e.what());
        return 0;
    }
}

int
luaMetaSetExpireTime(lua_State *lua) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 2);
        readMetaFromStack(lua, false);
        time_t expire_time = luaReadStack<time_t>(lua, 2);
        if (expire_time < 0) {
            throw std::runtime_error("negative expire time is not allowed");
        }
        InvokeContext* invoke_ctx = metaInvokeContext(lua, false);
        if (NULL != invoke_ctx) {
            invoke_ctx->meta()->setExpireTime(expire_time);
        }
        return 0;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        luaL_error(lua, "caught exception in meta.setExpireTime: %s", e.what());
        return 0;
    }
}

int
luaMetaGetLastModified(lua_State *lua) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 1);
        readMetaFromStack(lua, false);
        InvokeContext* invoke_ctx = metaInvokeContext(lua, false);
        lua_pushnumber(lua, invoke_ctx ?
            invoke_ctx->meta()->getLastModified() : Meta::undefinedLastModified());
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        luaL_error(lua, "caught exception in meta.getElapsedTime: %s", e.what());
        return 0;
    }
}

int
luaMetaSetLastModified(lua_State *lua) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 2);
        readMetaFromStack(lua, false);
        time_t last_modified = luaReadStack<time_t>(lua, 2);
        if (last_modified < 0) {
            throw std::runtime_error("negative last modified is not allowed");
        }
        InvokeContext* invoke_ctx = metaInvokeContext(lua, false);
        if (NULL != invoke_ctx) {
            invoke_ctx->meta()->setLastModified(last_modified);
        }
        return 0;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        luaL_error(lua, "caught exception in meta.setLastModified: %s", e.what());
        return 0;
    }
}

} // extern "C"

} // namespace xscript

