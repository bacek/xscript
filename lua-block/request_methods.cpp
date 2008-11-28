#include "settings.h"

#include <stdexcept>
#include <boost/bind.hpp>
#include <boost/current_function.hpp>
#include <boost/function.hpp>

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


template<typename Func> static int
lua_request_method(lua_State *lua, Func func) {
    try {
        luaCheckStackSize(lua, 2);
        log()->debug("%s: fetching request", BOOST_CURRENT_FUNCTION);
        Request *req = luaReadStack<Request>(lua, "xscript.request", 1);
        log()->debug("%s: fetching argument", BOOST_CURRENT_FUNCTION);
        std::string key = luaReadStack<std::string>(lua, 2);
        log()->debug("%s: invoking function with key %s", BOOST_CURRENT_FUNCTION, key.c_str());
        luaPushStack(lua, func(req, key));
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in args: %s", e.what());
    }
}

int
luaRequestGet(lua_State *lua, boost::function<std::string (xscript::Request*)> func) {
    try {
        luaCheckStackSize(lua, 1);
        log()->debug("%s: fetching request", BOOST_CURRENT_FUNCTION);
        Request *req = luaReadStack<Request>(lua, "xscript.request", 1);
        luaPushStack(lua, func(req));
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
    return lua_request_method(lua, boost::bind(&Request::getArg, _1, _2));
}

extern "C" int
luaRequestGetHeader(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    return lua_request_method(lua, boost::bind(&Request::getHeader, _1, _2));
}

extern "C" int
luaRequestGetCookie(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    return lua_request_method(lua, boost::bind(&Request::getCookie, _1, _2));
}

extern "C" int
luaRequestHasArg(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    return lua_request_method(lua, boost::bind(&Request::hasArg, _1, _2));
}

extern "C" int
luaRequestHasHeader(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    return lua_request_method(lua, boost::bind(&Request::hasHeader, _1, _2));
}

extern "C" int
luaRequestHasCookie(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    return lua_request_method(lua, boost::bind(&Request::hasCookie, _1, _2));
}

extern "C" int
luaRequestGetURI(lua_State *lua) throw () {
    return luaRequestGet(lua, boost::bind(&Request::getURI, _1));
}

extern "C" int
luaRequestGetPath(lua_State *lua) throw () {
    return luaRequestGet(lua, boost::bind(&Request::getScriptName, _1));
}

extern "C" int
luaRequestGetQuery(lua_State *lua) throw () {
    return luaRequestGet(lua, boost::bind(&Request::getQueryString, _1));
}

extern "C" int
luaRequestGetScriptName(lua_State *lua) throw () {
    return luaRequestGet(lua, boost::bind(&Request::getScriptName, _1));
}

extern "C" int
luaRequestGetMethod(lua_State *lua) throw () {
    return luaRequestGet(lua, boost::bind(&Request::getRequestMethod, _1));
}

extern "C" int
luaRequestGetRemoteIp(lua_State *lua) throw () {
    return luaRequestGet(lua, boost::bind(&Request::getRealIP, _1));
}

extern "C" int
luaRequestGetHostname(lua_State *lua) throw () {
    return luaRequestGet(lua, boost::bind(&Request::getHost, _1));
}

extern "C" int
luaRequestGetPathInfo(lua_State *lua) throw () {
    return luaRequestGet(lua, boost::bind(&Request::getPathInfo, _1));
}

extern "C" int
luaRequestGetRealPath(lua_State *lua) throw () {
    return luaRequestGet(lua, boost::bind(&Request::getPathTranslated, _1));
}

extern "C" int
luaRequestGetHTTPUser(lua_State *lua) throw () {
    return luaRequestGet(lua, boost::bind(&Request::getRemoteUser, _1));
}


//        luaRequestGetContentLength},

extern "C" int
luaRequestGetContentType(lua_State *lua) throw () {
    return luaRequestGet(lua, boost::bind(&Request::getContentType, _1));
}

extern "C" int
luaRequestGetContentEncoding(lua_State *lua) throw () {
    return luaRequestGet(lua, boost::bind(&Request::getContentEncoding, _1));
}

extern "C" int
luaRequestGetOriginalHost(lua_State *lua) throw () {
    return luaRequestGet(lua, boost::bind(&Request::getOriginalHost, _1));
}

extern "C" int
luaRequestGetOriginalURI(lua_State *lua) throw () {
    return luaRequestGet(lua, boost::bind(&Request::getOriginalURI, _1));
}

extern "C" int
luaRequestGetOriginalUrl(lua_State *lua) throw () {
    return luaRequestGet(lua, boost::bind(&Request::getOriginalUrl, _1));
}



static const struct luaL_reg requestlib [] = {
    {"getArg",        luaRequestGetArg},
    {"getHeader",     luaRequestGetHeader},
    {"getCookie",     luaRequestGetCookie},
    {"hasArg",        luaRequestHasArg},
    {"hasHeader",     luaRequestHasHeader},
    {"hasCookie",     luaRequestHasCookie},

    //{"isSecure",        luaRequestIsSecure},

    {"getURI",          luaRequestGetURI},
    {"getPath",         luaRequestGetPath},
    {"getQuery",        luaRequestGetQuery},
    {"getScriptName",   luaRequestGetScriptName},
    {"getMethod",       luaRequestGetMethod},
    {"getRemoteIp",     luaRequestGetRemoteIp},

    {"getHostname",     luaRequestGetHostname},
    {"getPathInfo",     luaRequestGetPathInfo},
    {"getRealPath",     luaRequestGetRealPath},
    {"getHTTPUser",     luaRequestGetHTTPUser},

    //{"getContentLength",    luaRequestGetContentLength},
    {"getContentType",      luaRequestGetContentType},
    {"getContentEncoding",  luaRequestGetContentEncoding},

    {"getOriginalHost", luaRequestGetOriginalHost},
    {"getOriginalURI",  luaRequestGetOriginalURI},
    {"getOriginalUrl",  luaRequestGetOriginalUrl},

    {NULL, NULL}
};

const struct luaL_reg * getRequestLib() {
    return requestlib;
}


} // namespace xscript
