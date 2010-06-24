#include "settings.h"

#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <boost/current_function.hpp>

#include "xscript/context.h"
#include "xscript/logger.h"
#include "xscript/state.h"
#include "xscript/string_utils.h"

#include "stack.h"
#include "exception.h"
#include "method_map.h"
#include "state_methods.h"
#include "xscript_methods.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

extern "C" {
    int luaStateHas(lua_State *lua);
    int luaStateGet(lua_State *lua);
    int luaStateGetTypedValue(lua_State *lua);

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
}

static const struct luaL_reg statelib [] = {
    {"has",             luaStateHas },
    {"get",             luaStateGet},
    {"getTypedValue",   luaStateGetTypedValue},
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
    {NULL, NULL}
};

const struct luaL_reg * getStateLib() {
    return statelib;
}

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
        if (3 == count) {
            def_value = luaReadStack<std::string>(lua, 3);
        }
        lua_pushstring(lua, state->asString(key, def_value).c_str());
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
        if (value.undefined()) {
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

int luaStateSetTable(lua_State *lua) {
    try {
        luaCheckStackSize(lua, 3);
        luaReadStack<void>(lua, "xscript.state", 1);
        State *state = getContext(lua)->state();
        std::string key = luaReadStack<std::string>(lua, 2);
        if (luaIsNil(lua, 3)) {
            return 0;
        }
        if (luaIsArrayTable(lua, 3)) {
            std::auto_ptr<std::vector<std::string> > value =
                luaReadStack<std::auto_ptr<std::vector<std::string> > >(lua, 3);
            state->set<const std::vector<std::string>&>(key, *value);
        }
        else {
            std::auto_ptr<std::map<std::string, std::string> > value =
                luaReadStack<std::auto_ptr<std::map<std::string, std::string> > >(lua, 3);
            state->set<const std::map<std::string, std::string>&>(key, *value);
        }
        return 0;
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

} // extern "C"

} // namespace xscript

