#include "settings.h"

#include <stdexcept>
#include <boost/bind.hpp>
#include <boost/current_function.hpp>
#include <functional>

#include "xscript/logger.h"
#include "xscript/request.h"

#include "stack.h"
#include "exception.h"
#include "method_map.h"
#include "logger_methods.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

extern "C" int luaLoggerCrit(lua_State *state) throw ();
extern "C" int luaLoggerError(lua_State *state) throw ();
extern "C" int luaLoggerWarn(lua_State *state) throw ();
extern "C" int luaLoggerInfo(lua_State *state) throw ();
extern "C" int luaLoggerDebug(lua_State *state) throw ();

static const struct luaL_reg loggerlib [] = {
    {"crit",          luaLoggerCrit},
    {"error",         luaLoggerError},
    {"warn",          luaLoggerWarn},
    {"info",          luaLoggerInfo},
    {"debug",         luaLoggerDebug},
    {NULL, NULL}
};

void registerLoggerMethods(lua_State *lua) {
    log()->debug("%s, >>>stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));

    //lua_sethook(lua, luaHook, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE, 1);

    const char* tableName = "xscript.logger";

    luaL_newmetatable(lua, tableName);
    lua_pushstring(lua, "__index");
    lua_pushvalue(lua, -2);  /* pushes the metatable */
    lua_settable(lua, -3);  /* metatable.__index = metatable */

    luaL_openlib(lua, tableName, loggerlib, 0);

    lua_pop(lua, 2); // pop 

    log()->debug("%s, <<<stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    return;
}

template<typename Func> int
luaLoggerFoo(lua_State *lua, Func func) {
    try {
        luaCheckStackSize(lua, 1);
        log()->debug("%s: fetching argument", BOOST_CURRENT_FUNCTION);
        std::string value = luaReadStack<std::string>(lua, 1);
        func(log(), "%s", value.c_str());
        //func(log(), "%s", value.c_str());
        return 0;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in args: %s", e.what());
    }
}

#define METHOD_BODY(METHOD)                                                 \
    try {                                                                   \
        luaCheckStackSize(lua, 1);                                          \
        log()->debug("%s: fetching argument", BOOST_CURRENT_FUNCTION);      \
        std::string value = luaReadStack<std::string>(lua, 1);              \
        log()->METHOD("%s", value.c_str());                                \
        return 0;                                                           \
    }                                                                       \
    catch (const LuaError &e) {                                             \
        return e.translate(lua);                                            \
    }                                                                       \
    catch (const std::exception &e) {                                       \
        return luaL_error(lua, "caught exception in args: %s", e.what());   \
    }

extern "C" int
luaLoggerCrit(lua_State *lua) throw () {
    METHOD_BODY(crit);
}

extern "C" int
luaLoggerError(lua_State *lua) throw () {
    METHOD_BODY(error);
}

extern "C" int
luaLoggerWarn(lua_State *lua) throw () {
    METHOD_BODY(warn);
}

extern "C" int
luaLoggerInfo(lua_State *lua) throw () {
    METHOD_BODY(info);
}

extern "C" int
luaLoggerDebug(lua_State *lua) throw () {
    METHOD_BODY(debug);
}


}

