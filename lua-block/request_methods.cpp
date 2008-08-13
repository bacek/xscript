#include "settings.h"

#include <stdexcept>
#include <boost/bind.hpp>
#include <boost/current_function.hpp>

#include "xscript/logger.h"
#include "xscript/request.h"

#include "stack.h"
#include "exception.h"
#include "method_map.h"
#include "request_methods.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

extern "C" int luaRequestGetArg(lua_State *state) throw ();
extern "C" int luaRequestGetHeader(lua_State *state) throw ();
extern "C" int luaRequestGetCookie(lua_State *state) throw ();
extern "C" int luaRequestHasArg(lua_State *state) throw ();
extern "C" int luaRequestHasHeader(lua_State *state) throw ();
extern "C" int luaRequestHasCookie(lua_State *state) throw ();

static const struct luaL_reg requestlib [] = {
    {"getArg",        luaRequestGetArg},
    {"getHeader",     luaRequestGetHeader},
    {"getCookie",     luaRequestGetCookie},
    {"hasArg",        luaRequestHasArg},
    {"hasHeader",     luaRequestHasHeader},
    {"hasCookie",     luaRequestHasCookie},
    {NULL, NULL}
};

const struct luaL_reg * getRequestLib() {
    return requestlib;
}


template<typename Func> int
luaRequestGet(lua_State *lua, Func func) {
    try {
        luaCheckStackSize(lua, 2);
        log()->debug("%s: fetching request", BOOST_CURRENT_FUNCTION);
        Request *req = luaReadStack<Request>(lua, "xscript.request", 1);
        log()->debug("%s: fetching argument", BOOST_CURRENT_FUNCTION);
        std::string key = luaReadStack<std::string>(lua, 2);
        log()->debug("%s: invoking function with key %s", BOOST_CURRENT_FUNCTION, key.c_str());
        const std::string &val = func(req, key);
        lua_pushstring(lua, val.c_str());
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in args: %s", e.what());
    }
}

extern "C" int
luaRequestGetArg(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    return luaRequestGet(lua, boost::bind(&Request::getArg, _1, _2));
}

extern "C" int
luaRequestGetHeader(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    return luaRequestGet(lua, boost::bind(&Request::getHeader, _1, _2));
}

extern "C" int
luaRequestGetCookie(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    return luaRequestGet(lua, boost::bind(&Request::getCookie, _1, _2));
}

// TODO Refactor Get and Has into one function.
template<typename Func> int
luaRequestHas(lua_State *lua, Func func) {
    try {
        luaCheckStackSize(lua, 2);
        log()->debug("%s: fetching request", BOOST_CURRENT_FUNCTION);
        Request *req = luaReadStack<Request>(lua, "xscript.request", 1);
        log()->debug("%s: fetching argument", BOOST_CURRENT_FUNCTION);
        std::string key = luaReadStack<std::string>(lua, 2);
        log()->debug("%s: invoking function with key %s", BOOST_CURRENT_FUNCTION, key.c_str());
        lua_pushboolean(lua, func(req, key));
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in args: %s", e.what());
    }
}

extern "C" int
luaRequestHasArg(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    return luaRequestHas(lua, boost::bind(&Request::hasArg, _1, _2));
}

extern "C" int
luaRequestHasHeader(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    return luaRequestHas(lua, boost::bind(&Request::hasHeader, _1, _2));
}

extern "C" int
luaRequestHasCookie(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    return luaRequestHas(lua, boost::bind(&Request::hasCookie, _1, _2));
}

} // namespace xscript
