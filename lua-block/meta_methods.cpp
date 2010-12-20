#include "settings.h"

#include <boost/current_function.hpp>

#include "xscript/block.h"
#include "xscript/context.h"
#include "xscript/logger.h"
#include "xscript/message_interface.h"
#include "xscript/meta.h"

#include "exception.h"
#include "lua_block.h"
#include "stack.h"
#include "xscript_methods.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

extern "C" {
    int luaMetaHas(lua_State *lua);
    int luaMetaGet(lua_State *lua);
    int luaMetaGetTypedValue(lua_State *lua);
    int luaMetaGetElapsedTime(lua_State *lua);
    int luaMetaGetExpireTime(lua_State *lua);
    int luaMetaSetExpireTime(lua_State *lua);
    int luaMetaGetLastModified(lua_State *lua);
    int luaMetaSetBool(lua_State *lua);
    int luaMetaSetLong(lua_State *lua);
    int luaMetaSetLongLong(lua_State *lua);
    int luaMetaSetULong(lua_State *lua);
    int luaMetaSetULongLong(lua_State *lua);
    int luaMetaSetDouble(lua_State *lua);
    int luaMetaSetString(lua_State *lua);
    int luaMetaSetTable(lua_State *lua);

    int luaSelfMetaHas(lua_State *lua);
    int luaSelfMetaGet(lua_State *lua);
    int luaSelfMetaGetTypedValue(lua_State *lua);
    int luaSelfMetaGetElapsedTime(lua_State *lua);
    int luaSelfMetaGetExpireTime(lua_State *lua);
    int luaSelfMetaSetExpireTime(lua_State *lua);
    int luaSelfMetaGetLastModified(lua_State *lua);
    int luaSelfMetaSetBool(lua_State *lua);
    int luaSelfMetaSetLong(lua_State *lua);
    int luaSelfMetaSetLongLong(lua_State *lua);
    int luaSelfMetaSetULong(lua_State *lua);
    int luaSelfMetaSetULongLong(lua_State *lua);
    int luaSelfMetaSetDouble(lua_State *lua);
    int luaSelfMetaSetString(lua_State *lua);
    int luaSelfMetaSetTable(lua_State *lua);
}

static const struct luaL_reg metalib [] = {
    {"has",             luaMetaHas},
    {"get",             luaMetaGet},
    {"getTypedValue",   luaMetaGetTypedValue},
    {"getElapsedTime",  luaMetaGetElapsedTime},
    {"getExpireTime",   luaMetaGetExpireTime},
    {"setExpireTime",   luaMetaSetExpireTime},
    {"getLastModified", luaMetaGetLastModified},
    {"setBool",         luaMetaSetBool},
    {"setBoolean",      luaMetaSetBool},
    {"setLong",         luaMetaSetLong},
    {"setLongLong",     luaMetaSetLongLong},
    {"setULong",        luaMetaSetULong},
    {"setULongLong",    luaMetaSetULongLong},
    {"setString",       luaMetaSetString},
    {"setDouble",       luaMetaSetDouble},
    {"setTable",        luaMetaSetTable},
    {NULL, NULL}
};

static const struct luaL_reg selfmetalib [] = {
    {"has",             luaSelfMetaHas},
    {"get",             luaSelfMetaGet},
    {"getTypedValue",   luaSelfMetaGetTypedValue},
    {"getElapsedTime",  luaSelfMetaGetElapsedTime},
    {"getExpireTime",   luaSelfMetaGetExpireTime},
    {"setExpireTime",   luaSelfMetaSetExpireTime},
    {"getLastModified", luaSelfMetaGetLastModified},
    {"setBool",         luaSelfMetaSetBool},
    {"setBoolean",      luaSelfMetaSetBool},
    {"setLong",         luaSelfMetaSetLong},
    {"setLongLong",     luaSelfMetaSetLongLong},
    {"setULong",        luaSelfMetaSetULong},
    {"setULongLong",    luaSelfMetaSetULongLong},
    {"setString",       luaSelfMetaSetString},
    {"setDouble",       luaSelfMetaSetDouble},
    {"setTable",        luaSelfMetaSetTable},
    {NULL, NULL}
};

static void
readMetaFromStack(lua_State *lua, bool self) {
    const char *name = self ? "xscript.selfmeta" : "xscript.meta";
    luaReadStack<void>(lua, name, 1);
}

static InvokeContext*
metaInvokeContext(lua_State *lua, bool self) {
    InvokeContext* invoke_ctx = getInvokeContext(lua);
    if (self && !invoke_ctx->isMeta()) {
        return invoke_ctx;
    }
    return invoke_ctx->parent(getContext(lua));
}

static int
luaMetaHasInternal(lua_State *lua, bool self) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 2);
        readMetaFromStack(lua, self);
        InvokeContext* invoke_ctx = metaInvokeContext(lua, self);
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
luaMetaGetInternal(lua_State *lua, bool self) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        int count = lua_gettop(lua);
        if (count < 2 || count > 3) {
            throw BadArgCount(count);
        }
        readMetaFromStack(lua, self);
        InvokeContext* invoke_ctx = metaInvokeContext(lua, self);
        std::string key = luaReadStack<std::string>(lua, 2);
        std::string def_value;
        bool is_nil = false;
        if (3 == count) {
            is_nil = luaIsNil(lua, 3);
            if (!is_nil) {
                def_value = luaReadStack<std::string>(lua, 3);
            }
        }
        if (is_nil && (NULL == invoke_ctx || !invoke_ctx->meta()->has(key))) {
            lua_pushnil(lua);
        }
        else {
            lua_pushstring(lua, invoke_ctx ?
                invoke_ctx->meta()->get(key, def_value).c_str() : def_value.c_str());
        }
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
luaMetaGetTypedValueInternal(lua_State *lua, bool self) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 2);
        readMetaFromStack(lua, self);
        InvokeContext* invoke_ctx = metaInvokeContext(lua, self);
        std::string key = luaReadStack<std::string>(lua, 2);
        if (NULL == invoke_ctx) {
            lua_pushnil(lua);
            return 1;
        }
        const TypedValue& value = invoke_ctx->meta()->getTypedValue(key);
        if (value.nil()) {
            lua_pushnil(lua);
            return 1;
        }
        luaPushStack<const TypedValue&>(lua, value);
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        luaL_error(lua, "caught exception in meta.getTypedValue: %s", e.what());
        return 0;
    }
}

template<typename Type> int
luaMetaSet(lua_State *lua, bool self) {
    try {
        luaCheckStackSize(lua, 3);
        readMetaFromStack(lua, self);
        InvokeContext* invoke_ctx = metaInvokeContext(lua, self);
        std::string key = luaReadStack<std::string>(lua, 2);
        Type value = luaReadStack<Type>(lua, 3);
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
        log()->debug("caught exception in meta.set: %s", e.what());
        return luaL_error(lua, "caught exception in meta.set: %s", e.what());
    }
}

static int
luaMetaSetBoolInternal(lua_State *lua, bool self) {
    return luaMetaSet<bool>(lua, self);
}

static int
luaMetaSetLongInternal(lua_State *lua, bool self) {
    return luaMetaSet<boost::int32_t>(lua, self);
}

static int
luaMetaSetLongLongInternal(lua_State *lua, bool self) {
    return luaMetaSet<boost::int64_t>(lua, self);
}

static int
luaMetaSetULongInternal(lua_State *lua, bool self) {
    return luaMetaSet<boost::uint32_t>(lua, self);
}

static int
luaMetaSetULongLongInternal(lua_State *lua, bool self) {
    return luaMetaSet<boost::uint64_t>(lua, self);
}

static int
luaMetaSetStringInternal(lua_State *lua, bool self) {
    return luaMetaSet<std::string>(lua, self);
}

static int
luaMetaSetDoubleInternal(lua_State *lua, bool self) {
    return luaMetaSet<double>(lua, self);
}

static int
luaMetaSetTableInternal(lua_State *lua, bool self) {
    try {
        luaCheckStackSize(lua, 3);
        readMetaFromStack(lua, self);
        InvokeContext* invoke_ctx = metaInvokeContext(lua, self);
        if (!luaIsNil(lua, 3) && invoke_ctx) {
            std::string key = luaReadStack<std::string>(lua, 2);
            invoke_ctx->meta()->setTypedValue(key, luaReadTable(lua, 3));
        }
        return 0;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        log()->debug("caught exception in meta.setTable: %s", e.what());
        return luaL_error(lua, "caught exception in meta.setTable: %s", e.what());
    }
}

static int
luaMetaGetElapsedTimeInternal(lua_State *lua, bool self) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 1);
        readMetaFromStack(lua, self);
        InvokeContext* invoke_ctx = metaInvokeContext(lua, self);
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

static int
luaMetaGetExpireTimeInternal(lua_State *lua, bool self) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 1);
        readMetaFromStack(lua, self);
        InvokeContext* invoke_ctx = metaInvokeContext(lua, self);
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

static int
luaMetaSetExpireTimeInternal(lua_State *lua, bool self) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 2);
        readMetaFromStack(lua, self);
        time_t expire_time = luaReadStack<time_t>(lua, 2);
        if (expire_time < 0) {
            throw std::runtime_error("negative expire time is not allowed");
        }
        InvokeContext* invoke_ctx = metaInvokeContext(lua, self);
        if (NULL != invoke_ctx) {
            invoke_ctx->meta()->setExpireTime(expire_time);
        }
        lua_pushnumber(lua, invoke_ctx ?
            invoke_ctx->meta()->getExpireTime() : Meta::undefinedExpireTime());
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        luaL_error(lua, "caught exception in meta.setExpireTime: %s", e.what());
        return 0;
    }
}

static int
luaMetaGetLastModifiedInternal(lua_State *lua, bool self) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 1);
        readMetaFromStack(lua, self);
        InvokeContext* invoke_ctx = metaInvokeContext(lua, self);
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

extern "C" {

int
luaMetaHas(lua_State *lua) {
    return luaMetaHasInternal(lua, false);
}

int
luaSelfMetaHas(lua_State *lua) {
    return luaMetaHasInternal(lua, true);
}

int
luaMetaGet(lua_State *lua) {
    return luaMetaGetInternal(lua, false);
}

int
luaSelfMetaGet(lua_State *lua) {
    return luaMetaGetInternal(lua, true);
}

int
luaMetaGetTypedValue(lua_State *lua) {
    return luaMetaGetTypedValueInternal(lua, false);
}

int
luaSelfMetaGetTypedValue(lua_State *lua) {
    return luaMetaGetTypedValueInternal(lua, true);
}

int
luaMetaGetElapsedTime(lua_State *lua) {
    return luaMetaGetElapsedTimeInternal(lua, false);
}

int
luaSelfMetaGetElapsedTime(lua_State *lua) {
    return luaMetaGetElapsedTimeInternal(lua, true);
}

int
luaMetaGetExpireTime(lua_State *lua) {
    return luaMetaGetExpireTimeInternal(lua, false);
}

int
luaSelfMetaGetExpireTime(lua_State *lua) {
    return luaMetaGetExpireTimeInternal(lua, true);
}

int
luaMetaSetExpireTime(lua_State *lua) {
    return luaMetaSetExpireTimeInternal(lua, false);
}

int
luaSelfMetaSetExpireTime(lua_State *lua) {
    return luaMetaSetExpireTimeInternal(lua, true);
}

int
luaMetaGetLastModified(lua_State *lua) {
    return luaMetaGetLastModifiedInternal(lua, false);
}

int
luaSelfMetaGetLastModified(lua_State *lua) {
    return luaMetaGetLastModifiedInternal(lua, true);
}

int
luaMetaSetBool(lua_State *lua) {
    return luaMetaSetBoolInternal(lua, false);
}

int
luaMetaSetLong(lua_State *lua) {
    return luaMetaSetLongInternal(lua, false);
}

int
luaMetaSetLongLong(lua_State *lua) {
    return luaMetaSetLongLongInternal(lua, false);
}

int
luaMetaSetULong(lua_State *lua) {
    return luaMetaSetULongInternal(lua, false);
}

int
luaMetaSetULongLong(lua_State *lua) {
    return luaMetaSetULongLongInternal(lua, false);
}

int
luaMetaSetDouble(lua_State *lua) {
    return luaMetaSetDoubleInternal(lua, false);
}

int
luaMetaSetString(lua_State *lua) {
    return luaMetaSetStringInternal(lua, false);
}

int
luaMetaSetTable(lua_State *lua) {
    return luaMetaSetTableInternal(lua, false);
}

int
luaSelfMetaSetBool(lua_State *lua) {
    return luaMetaSetBoolInternal(lua, true);
}

int
luaSelfMetaSetLong(lua_State *lua) {
    return luaMetaSetLongInternal(lua, true);
}

int
luaSelfMetaSetLongLong(lua_State *lua) {
    return luaMetaSetLongLongInternal(lua, true);
}

int
luaSelfMetaSetULong(lua_State *lua) {
    return luaMetaSetULongInternal(lua, true);
}

int
luaSelfMetaSetULongLong(lua_State *lua) {
    return luaMetaSetULongLongInternal(lua, true);
}

int
luaSelfMetaSetDouble(lua_State *lua) {
    return luaMetaSetDoubleInternal(lua, true);
}

int
luaSelfMetaSetString(lua_State *lua) {
    return luaMetaSetStringInternal(lua, true);
}

int
luaSelfMetaSetTable(lua_State *lua) {
    return luaMetaSetTableInternal(lua, true);
}

} // extern "C"

class LuaMetaRegisterHandler : public MessageHandler {
    Result process(const MessageParams &params, MessageResultBase &result) {
        (void)result;
        std::vector<LuaExtension::LuaRegisterFunc>* registerers =
                params.getPtr<std::vector<LuaExtension::LuaRegisterFunc> >(0);
        if (registerers) {
            registerers->push_back(boost::bind(&LuaExtension::registerLib,
                    _1, "meta", true, metalib, metalib));
        }
        return CONTINUE;
    }
};

class LuaSelfMetaRegisterHandler : public MessageHandler {
    Result process(const MessageParams &params, MessageResultBase &result) {
        (void)result;
        std::vector<LuaExtension::LuaRegisterFunc>* registerers =
                params.getPtr<std::vector<LuaExtension::LuaRegisterFunc> >(0);
        if (registerers) {
            registerers->push_back(boost::bind(&LuaExtension::registerLib,
                    _1, "selfmeta", true, selfmetalib, selfmetalib));
        }
        return CONTINUE;
    }
};

class LuaMetaHandlerRegisterer {
public:
    LuaMetaHandlerRegisterer() {
        MessageProcessor::instance()->registerBack("REGISTER_LUA_EXTENSION",
            boost::shared_ptr<MessageHandler>(new LuaMetaRegisterHandler()));
        MessageProcessor::instance()->registerBack("REGISTER_LUA_EXTENSION",
            boost::shared_ptr<MessageHandler>(new LuaSelfMetaRegisterHandler()));
    }
};

static LuaMetaHandlerRegisterer reg_lua_meta_handlers;


} // namespace xscript

