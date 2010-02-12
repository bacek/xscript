#include "settings.h"

#include <stdexcept>
#include <boost/current_function.hpp>

#include "xscript/context.h"
#include "xscript/logger.h"
#include "xscript/response.h"

#include "stack.h"
#include "exception.h"
#include "method_map.h"
#include "response_methods.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

extern "C" int luaResponseSetStatus(lua_State *lua) throw ();
extern "C" int luaResponseSetHeader(lua_State *lua) throw ();
extern "C" int luaResponseSetCookie(lua_State *lua) throw ();
extern "C" int luaResponseRedirectToPath(lua_State *lua) throw ();
extern "C" int luaResponseSetContentType(lua_State *lua) throw ();
extern "C" int luaResponseSetExpireTimeDelta(lua_State *lua) throw ();

static const struct luaL_reg responselib [] = {
    {"setStatus",       luaResponseSetStatus},
    {"setHeader",       luaResponseSetHeader},
    {"setCookie",       luaResponseSetCookie},
    {"redirectToPath",  luaResponseRedirectToPath},
    {"setContentType",  luaResponseSetContentType},
    {"setExpireTimeDelta",  luaResponseSetExpireTimeDelta},
    {NULL, NULL}
};

const struct luaL_reg * getResponseLib() {
    return responselib;
}

extern "C" int
luaResponseSetStatus(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 2);
        Response *response = luaReadStack<Response>(lua, "xscript.response", 1);
        unsigned short status = luaReadStack<boost::uint32_t>(lua, 2);
        response->setStatus(status);
        return 0;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in response.setStatus: %s", e.what());
    }
}

extern "C" int
luaResponseSetHeader(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 3);
        Response *response = luaReadStack<Response>(lua, "xscript.response", 1);
        response->setHeader(
            luaReadStack<std::string>(lua, 2), luaReadStack<std::string>(lua, 3));
        return 0;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in response.setHeader: %s", e.what());
    }
}

extern "C" int
luaResponseRedirectToPath(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 2);
        Response *response = luaReadStack<Response>(lua, "xscript.response", 1);
        std::string value = luaReadStack<std::string>(lua, 2);
        response->redirectToPath(value);
        return 0;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in response.setStatus: %s", e.what());
    }
}

extern "C" int
luaResponseSetContentType(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 2);
        Response *response = luaReadStack<Response>(lua, "xscript.response", 1);
        std::string value = luaReadStack<std::string>(lua, 2);
        response->setContentType(value);
        return 0;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in response.setStatus: %s", e.what());
    }
}

extern "C" int
luaResponseSetCookie(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 2);
        Response *response = luaReadStack<Response>(lua, "xscript.response", 1);
        Cookie * c = luaReadStack<Cookie>(lua, "xscript.cookie", 2);
        response->setCookie(*c);
        return 0;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in response.setStatus: %s", e.what());
    }
    return 0;
}

extern "C" int
luaResponseSetExpireTimeDelta(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 2);
        Response *response = luaReadStack<Response>(lua, "xscript.response", 1);
        boost::int32_t expire_time_delta = luaReadStack<boost::int32_t>(lua, 2);
        if (expire_time_delta < 0) {
            throw std::runtime_error("negative expire time delta is not allowed");
        }
        response->setExpireDelta((boost::uint32_t)expire_time_delta);
        return 0;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in response.setExpireTimeDelta: %s", e.what());
    }
}

} // namespace xscript
