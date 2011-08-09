#include "settings.h"

#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <boost/current_function.hpp>

#include "xscript/context.h"
#include "xscript/logger.h"
#include "xscript/message_interface.h"
#include "xscript/state.h"
#include "xscript/string_utils.h"

#include "lua_block.h"
#include "stack.h"
#include "exception.h"
#include "lua_block.h"
#include "method_map.h"
#include "xscript_methods.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

extern "C" {
    int luaStateHas(lua_State *lua);
    int luaStateGet(lua_State *lua);
    int luaStateGetTypedValue(lua_State *lua);
    int luaStateGetAll(lua_State *lua);

    int luaStateSetBool(lua_State *lua);
    int luaStateSetLong(lua_State *lua);
    int luaStateSetLongLong(lua_State *lua);
    int luaStateSetULong(lua_State *lua);
    int luaStateSetULongLong(lua_State *lua);
    int luaStateSetDouble(lua_State *lua);
    int luaStateSetString(lua_State *lua);
    int luaStateSetTable(lua_State *lua);

    int luaStateIs(lua_State *lua);
    int luaStateDrop(lua_State *lua);
    int luaStateErase(lua_State *lua);
}

static const struct luaL_reg statelib [] = {
    {"has",             luaStateHas },
    {"get",             luaStateGet},
    {"getTypedValue",   luaStateGetTypedValue},
    {"getAll",          luaStateGetAll},
    {"setBool",         luaStateSetBool},
    {"setBoolean",      luaStateSetBool},
    {"setLong",         luaStateSetLong},
    {"setLongLong",     luaStateSetLongLong},
    {"setULong",        luaStateSetULong},
    {"setULongLong",    luaStateSetULongLong},
    {"setString",       luaStateSetString},
    {"setDouble",       luaStateSetDouble},
    {"setTable",        luaStateSetTable},
    {"is",              luaStateIs},
    {"drop",            luaStateDrop},
    {"erase",           luaStateErase},
    {NULL, NULL}
};

extern "C" {

int
luaStateHas(lua_State *lua) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 2);
        luaReadStack<void>(lua, "xscript.state", 1);
        State *state = getContext(lua)->state();
        std::string key = luaReadStack<std::string>(lua, 2);
        log()->debug("luaStateHas: %s", key.c_str());
        lua_pushboolean(lua, state->has(key));
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        luaL_error(lua, "caught exception in state.has: %s", e.what());
        return 0;
    }
}

int
luaStateGet(lua_State *lua) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        int count = lua_gettop(lua);
        if (count < 2 || count > 3) {
            throw BadArgCount(count);
        }
        luaReadStack<void>(lua, "xscript.state", 1);
        State *state = getContext(lua)->state();
        std::string key = luaReadStack<std::string>(lua, 2);
        log()->debug("luaStateGet: %s", key.c_str());
        std::string def_value;
        bool is_nil = false;
        if (3 == count) {
            is_nil = luaIsNil(lua, 3);
            if (!is_nil) {
                def_value = luaReadStack<std::string>(lua, 3);
            }
        }
        if (is_nil && !state->has(key)) {
            lua_pushnil(lua);
        }
        else {
            lua_pushstring(lua, state->asString(key, def_value).c_str());
        }
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        luaL_error(lua, "caught exception in state.get: %s", e.what());
        return 0;
    }
}

int
luaStateGetTypedValue(lua_State *lua) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 2);
        luaReadStack<void>(lua, "xscript.state", 1);
        State *state = getContext(lua)->state();
        std::string key = luaReadStack<std::string>(lua, 2);
        log()->debug("luaStateGet: %s", key.c_str());
        TypedValue value = state->typedValue(key);
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
        luaL_error(lua, "caught exception in state.getTypedValue: %s", e.what());
        return 0;
    }
}

int
luaStateGetAll(lua_State *lua) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 1);
        luaReadStack<void>(lua, "xscript.state", 1);
        std::map<std::string, TypedValue> values;
        getContext(lua)->state()->values(values);
        luaPushStack<const std::map<std::string, TypedValue>&>(lua, values);
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        luaL_error(lua, "caught exception in state.getAll: %s", e.what());
        return 0;
    }
}

} // extern "C"


template<typename Type> int
luaStateSet(lua_State *lua) {
    try {
        luaCheckStackSize(lua, 3);
        luaReadStack<void>(lua, "xscript.state", 1);
        State *state = getContext(lua)->state();
        std::string key = luaReadStack<std::string>(lua, 2);
        Type value = luaReadStack<Type>(lua, 3);
        log()->debug("luaStateSet: %s", key.c_str());
        state->set(key, value);
        luaPushStack(lua, value);
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        log()->debug("caught exception in state.set: %s", e.what());
        return luaL_error(lua, "caught exception in state.set: %s", e.what());
    }
}

extern "C" {

int
luaStateSetBool(lua_State *lua) {
    return luaStateSet<bool>(lua);
}

int
luaStateSetLong(lua_State *lua) {
    return luaStateSet<boost::int32_t>(lua);
}

int
luaStateSetLongLong(lua_State *lua) {
    return luaStateSet<boost::int64_t>(lua);
}

int
luaStateSetULong(lua_State *lua) {
    return luaStateSet<boost::uint32_t>(lua);
}

int
luaStateSetULongLong(lua_State *lua) {
    return luaStateSet<boost::uint64_t>(lua);
}

int
luaStateSetString(lua_State *lua) {
    return luaStateSet<std::string>(lua);
}

int
luaStateSetDouble(lua_State *lua) {
    return luaStateSet<double>(lua);
}

int
luaStateSetTable(lua_State *lua) {
    try {
        luaCheckStackSize(lua, 3);
        luaReadStack<void>(lua, "xscript.state", 1);
        State *state = getContext(lua)->state();
        if (luaIsNil(lua, 3)) {
            lua_pushnil(lua);
        }
        else {
            std::string key = luaReadStack<std::string>(lua, 2);
            TypedValue table = luaReadTable(lua, 3);
            state->setTypedValue(key, table);
            luaPushStack<const TypedValue &>(lua, table);
        }
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        log()->debug("caught exception in state.setTable: %s", e.what());
        return luaL_error(lua, "caught exception in state.setTable: %s", e.what());
    }
}

int
luaStateIs(lua_State *lua) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 2);
        luaReadStack<void>(lua, "xscript.state", 1);
        State *state = getContext(lua)->state();
        std::string key = luaReadStack<std::string>(lua, 2);
        log()->debug("luaStateIs: %s", key.c_str());
        lua_pushboolean(lua, state->is(key));
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        luaL_error(lua, "caught exception in state.is: %s", e.what());
        return 0;
    }
}

int
luaStateDrop(lua_State *lua) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        int count = luaCheckStackSize(lua, 1, 2);
        luaReadStack<void>(lua, "xscript.state", 1);
        State *state = getContext(lua)->state();
        std::string prefix = count == 2 ?
            luaReadStack<std::string>(lua, 2) : StringUtils::EMPTY_STRING;
        log()->debug("luaStateDrop: %s", prefix.c_str());
        prefix.empty() ? state->clear() : state->erasePrefix(prefix);
        return 0;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        luaL_error(lua, "caught exception in state.drop: %s", e.what());
        return 0;
    }
}

int
luaStateErase(lua_State *lua) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 2);
        luaReadStack<void>(lua, "xscript.state", 1);
        State *state = getContext(lua)->state();
        std::string key = luaReadStack<std::string>(lua, 2);
        if (!key.empty()) {
            log()->debug("luaStateErase: %s", key.c_str());
            state->erase(key);
        }
        return 0;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        luaL_error(lua, "caught exception in state.erase: %s", e.what());
        return 0;
    }
}

} // extern "C"

class LuaStateRegisterHandler : public MessageHandler {
    Result process(const MessageParams &params, MessageResultBase &result) {
        (void)result;
        std::vector<LuaExtension::LuaRegisterFunc>* registerers =
                params.getPtr<std::vector<LuaExtension::LuaRegisterFunc> >(0);
        if (registerers) {
            registerers->push_back(boost::bind(&LuaExtension::registerLib,
                    _1, "state", true, statelib, statelib));
        }
        return CONTINUE;
    }
};

class LuaStateHandlerRegisterer {
public:
    LuaStateHandlerRegisterer() {
        MessageProcessor::instance()->registerBack("REGISTER_LUA_EXTENSION",
            boost::shared_ptr<MessageHandler>(new LuaStateRegisterHandler()));
    }
};

static LuaStateHandlerRegisterer reg_lua_state_handlers;

} // namespace xscript

