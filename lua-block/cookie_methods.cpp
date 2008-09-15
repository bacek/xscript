#include "settings.h"

#include <boost/current_function.hpp>
#include "xscript/logger.h"
#include "xscript/cookie.h"

#include "stack.h"
#include "cookie_methods.h"

extern "C" {
    int luaCookieNew(lua_State *);
    int luaCookieName(lua_State *);
    int luaCookieValue(lua_State *);
    int luaCookieSecure(lua_State *);
    int luaCookieExpires(lua_State *);
    int luaCookiePath(lua_State *);
    int luaCookieDomain(lua_State *);
    int luaCookiePermanent(lua_State *);

    int luaCookieDelete(lua_State *);
}

static const struct luaL_reg cookielib_f [] = {
    {"new",     luaCookieNew},
    {NULL,      NULL}
};

static const struct luaL_reg cookielib_m [] = {
    {"__gc",        luaCookieDelete },      // Destroy cookie

    {"name",        luaCookieName},
    {"value",       luaCookieValue},
    {"secure",      luaCookieSecure},
    {"expires",     luaCookieExpires},
    {"path",        luaCookiePath},
    {"domain",      luaCookieDomain},
    {"permanent",   luaCookiePermanent},
    {NULL, NULL}
};

namespace xscript {


void registerCookieMethods(lua_State *lua) {
    log()->debug("%s, >>>stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));

    //lua_sethook(lua, luaHook, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE, 1);

    const char* tableName = "xscript.cookie";

    luaL_newmetatable(lua, tableName);
    lua_pushstring(lua, "__index");
    lua_pushvalue(lua, -2);  /* pushes the metatable */
    lua_settable(lua, -3);  /* metatable.__index = metatable */

    luaL_openlib(lua, NULL, cookielib_m, 0);
    luaL_openlib(lua, tableName, cookielib_f, 0);

    lua_pop(lua, 2); // pop 

    log()->debug("%s, <<<stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    return;
}


extern "C" {

int luaCookieNew(lua_State * lua) {
    log()->debug("%s, >>>stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));

    luaCheckStackSize(lua, 2);
    std::string name = luaReadStack<std::string>(lua, 1);
    std::string value = luaReadStack<std::string>(lua, 2);

    size_t nbytes = sizeof(pointer<Cookie>);
    pointer<Cookie> *a = (pointer<Cookie> *)lua_newuserdata(lua, nbytes);
    a->ptr = new Cookie(name, value);

    /* set its metatable */
    luaL_getmetatable(lua, "xscript.cookie");
    lua_setmetatable(lua, -2);

    log()->debug("%s, <<<stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));

    return 1;  /* new userdatum is already on the stack */
}

int luaCookieDelete(lua_State * lua) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));

    Cookie * c = luaReadStack<Cookie>(lua, "xscript.cookie", 1);
    delete c;
    return 0;
}

int luaCookieName(lua_State * lua) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));

    luaCheckStackSize(lua, 1);
    Cookie * c = luaReadStack<Cookie>(lua, "xscript.cookie", 1);
    lua_pushstring(lua, c->name().c_str());
    return 1;
}

int luaCookieValue(lua_State * lua) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));

    luaCheckStackSize(lua, 1);
    Cookie * c = luaReadStack<Cookie>(lua, "xscript.cookie", 1);
    lua_pushstring(lua, c->value().c_str());
    return 1;
}

int luaCookieSecure(lua_State * lua) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    int size = lua_gettop(lua);
    Cookie * c = luaReadStack<Cookie>(lua, "xscript.cookie", 1);
    if (size == 1) {
        lua_pushboolean(lua, c->secure());
        return 1;
    }
    else if (size == 2) {
        bool value = luaReadStack<bool>(lua, 2);
        c->secure(value);
        return 0;
    }
    luaL_error(lua, "Invalid arity");
    return 0;
}

int luaCookieExpires(lua_State * lua) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    int size = lua_gettop(lua);
    Cookie * c = luaReadStack<Cookie>(lua, "xscript.cookie", 1);
    if (size == 1) {
        lua_pushnumber(lua, c->expires());
        return 1;
    }
    else if (size == 2) {
        time_t value = luaReadStack<time_t>(lua, 2);
        c->expires(value);
        return 0;
    }
    luaL_error(lua, "Invalid arity");
    return 0;
}

int luaCookiePath(lua_State * lua) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    int size = lua_gettop(lua);
    Cookie * c = luaReadStack<Cookie>(lua, "xscript.cookie", 1);
    if (size == 1) {
        lua_pushstring(lua, c->path().c_str());
        return 1;
    }
    else if (size == 2) {
        std::string value = luaReadStack<std::string>(lua, 2);
        c->path(value);
        return 0;
    }
    luaL_error(lua, "Invalid arity");
    return 0;
}

int luaCookieDomain(lua_State * lua) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    int size = lua_gettop(lua);
    Cookie * c = luaReadStack<Cookie>(lua, "xscript.cookie", 1);
    if (size == 1) {
        lua_pushstring(lua, c->domain().c_str());
        return 1;
    }
    else if (size == 2) {
        std::string value = luaReadStack<std::string>(lua, 2);
        c->domain(value);
        return 0;
    }
    luaL_error(lua, "Invalid arity");
    return 0;
}

int luaCookiePermanent(lua_State * lua) {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    int size = lua_gettop(lua);
    Cookie * c = luaReadStack<Cookie>(lua, "xscript.cookie", 1);
    if (size == 1) {
        lua_pushboolean(lua, c->permanent());
        return 1;
    }
    else if (size == 2) {
        bool value = luaReadStack<bool>(lua, 2);
        c->permanent(value);
        return 0;
    }
    luaL_error(lua, "Invalid arity");
    return 0;
}


} // extern "C"

}
