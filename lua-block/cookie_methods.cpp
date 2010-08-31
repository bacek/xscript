#include "settings.h"

#include <boost/current_function.hpp>
#include "xscript/logger.h"
#include "xscript/cookie.h"

#include "stack.h"
#include "cookie_methods.h"
#include "exception.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

extern "C" {
    int luaCookieNew(lua_State *);
    int luaCookieName(lua_State *);
    int luaCookieValue(lua_State *);
    int luaCookieSecure(lua_State *);
    int luaCookieHttpOnly(lua_State *);
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
    {"httpOnly",    luaCookieHttpOnly},
    {"expires",     luaCookieExpires},
    {"path",        luaCookiePath},
    {"domain",      luaCookieDomain},
    {"permanent",   luaCookiePermanent},
    {NULL, NULL}
};

extern "C" {

int luaCookieNew(lua_State * lua) {
    try {
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
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in args: %s", e.what());
    }
}

int luaCookieDelete(lua_State * lua) {
    try {
        log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));

        Cookie * c = luaReadStack<Cookie>(lua, "xscript.cookie", 1);
        // XXX It's looks suspicious.
        delete c;
        return 0;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in args: %s", e.what());
    }
}

int luaCookieName(lua_State * lua) {
    try {
        log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));

        luaCheckStackSize(lua, 1);
        Cookie * c = luaReadStack<Cookie>(lua, "xscript.cookie", 1);
        lua_pushstring(lua, c->name().c_str());
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in args: %s", e.what());
    }
}

int luaCookieValue(lua_State * lua) {
    try {
        log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));

        luaCheckStackSize(lua, 1);
        Cookie * c = luaReadStack<Cookie>(lua, "xscript.cookie", 1);
        lua_pushstring(lua, c->value().c_str());
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in args: %s", e.what());
    }
}

int luaCookieSecure(lua_State * lua) {
    try {
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
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in args: %s", e.what());
    }
}

int luaCookieHttpOnly(lua_State *lua) {
    try {
        log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
        int size = lua_gettop(lua);
        Cookie* c = luaReadStack<Cookie>(lua, "xscript.cookie", 1);
        if (size == 1) {
            lua_pushboolean(lua, c->httpOnly());
            return 1;
        }
        else if (size == 2) {
            bool value = luaReadStack<bool>(lua, 2);
            c->httpOnly(value);
            return 0;
        }
        luaL_error(lua, "Invalid arity");
        return 0;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in args: %s", e.what());
    }
}

int luaCookieExpires(lua_State * lua) {
    try {
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
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in args: %s", e.what());
    }
}

int luaCookiePath(lua_State * lua) {
    try {
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
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in args: %s", e.what());
    }
}

int luaCookieDomain(lua_State * lua) {
    try {
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
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in args: %s", e.what());
    }
}

int luaCookiePermanent(lua_State * lua) {
    try {
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
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in args: %s", e.what());
    }
}

} // extern "C"

const struct luaL_reg * getCookieNewLib() {
    return cookielib_f;
}

const struct luaL_reg * getCookieLib() {
    return cookielib_m;
}

}
