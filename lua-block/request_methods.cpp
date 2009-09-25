#include "settings.h"

#include <stdexcept>
#include <boost/bind.hpp>
#include <boost/current_function.hpp>
#include <boost/function.hpp>
#include <functional>

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

template<int ArgCount>
struct lua_request_method;

template<>
struct lua_request_method<1> {
    template<typename Func>
    static int invoke(lua_State *lua, Func func) {
        try {
            luaCheckStackSize(lua, 1);
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
};

template<>
struct lua_request_method<2> {
    template<typename Func>
    static int invoke(lua_State *lua, Func func) {
        try {
            luaCheckStackSize(lua, 2);
            Request *req = luaReadStack<Request>(lua, "xscript.request", 1);
            std::string key = luaReadStack<std::string>(lua, 2);
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
};

// Automatically deduce template specialisation to call.
template<typename Class, typename Ret>
int
call_method(lua_State *lua, Ret (Class::*func)()) {
    return lua_request_method<1>::invoke(lua, std::mem_fun(func));
}

template<typename Class, typename Ret>
int
call_method(lua_State *lua, Ret (Class::*func)() const) {
    return lua_request_method<1>::invoke(lua, std::mem_fun(func));
}

template<typename Class, typename Ret, typename A0>
int
call_method(lua_State *lua, Ret (Class::*func)(A0)) {
    return lua_request_method<2>::invoke(lua, std::mem_fun(func));
}

template<typename Class, typename Ret, typename A0>
int
call_method(lua_State *lua, Ret (Class::*func)(A0) const) {
    return lua_request_method<2>::invoke(lua, std::mem_fun(func));
}

extern "C" int
luaRequestGetArg(lua_State *lua) throw () {
    return call_method(lua, &Request::getArg);
}

static std::auto_ptr<std::vector<std::string> >
get_args(Request *request, const std::string &name) {
    std::auto_ptr<std::vector<std::string> > args(new std::vector<std::string>);
    std::vector<std::string> names;
    request->getArg(name, *args);
    return args;
}


static std::auto_ptr<std::map<std::string, std::string> > 
get_args_all(Request *request) { 
    std::auto_ptr<std::map<std::string, std::string> > args(new std::map<std::string, std::string>); 
    std::vector<std::string> names; 
    request->argNames(names); 
    for(std::vector<std::string>::const_iterator i = names.begin(), end = names.end(); i != end; ++i) { 
        args->insert(std::make_pair(*i, request->getArg(*i))); 
    } 
    return args; 
} 


extern "C" int
luaRequestGetArgs(lua_State *lua) throw () {
    if (lua_gettop(lua) == 2) {
        return lua_request_method<2>::invoke(lua, get_args);
    }
    return lua_request_method<1>::invoke(lua, get_args_all);
}

extern "C" int
luaRequestGetHeader(lua_State *lua) throw () {
    return call_method(lua, &Request::getHeader);
}

extern "C" int
luaRequestGetCookie(lua_State *lua) throw () {
    return call_method(lua, &Request::getCookie);
}

extern "C" int
luaRequestHasArg(lua_State *lua) throw () {
    return call_method(lua, &Request::hasArg);
}

extern "C" int
luaRequestHasHeader(lua_State *lua) throw () {
    return call_method(lua, &Request::hasHeader);
}

extern "C" int
luaRequestHasCookie(lua_State *lua) throw () {
    return call_method(lua, &Request::hasCookie);
}

extern "C" int
luaRequestGetURI(lua_State *lua) throw () {
    return call_method(lua, &Request::getURI);
}

extern "C" int
luaRequestGetPath(lua_State *lua) throw () {
    return call_method(lua, &Request::getScriptName);
}

extern "C" int
luaRequestGetQuery(lua_State *lua) throw () {
    return call_method(lua, &Request::getQueryString);
}

extern "C"
int
luaRequestIsSecure(lua_State *lua) throw () {
    return call_method(lua, &Request::isSecure);
}

extern "C"
int
luaRequestIsBot(lua_State *lua) throw () {
    return call_method(lua, &Request::isBot);
}

extern "C" int
luaRequestGetScriptName(lua_State *lua) throw () {
    return call_method(lua, &Request::getScriptName);
}

extern "C" int
luaRequestGetMethod(lua_State *lua) throw () {
    return call_method(lua, &Request::getRequestMethod);
}

extern "C" int
luaRequestGetRemoteIp(lua_State *lua) throw () {
    return call_method(lua, &Request::getRealIP);
}

extern "C" int
luaRequestGetHost(lua_State *lua) throw () {
    return call_method(lua, &Request::getHost);
}

extern "C" int
luaRequestGetPathInfo(lua_State *lua) throw () {
    return call_method(lua, &Request::getPathInfo);
}

extern "C" int
luaRequestGetRealPath(lua_State *lua) throw () {
    return call_method(lua, &Request::getPathTranslated);
}

extern "C" int
luaRequestGetHTTPUser(lua_State *lua) throw () {
    return call_method(lua, &Request::getRemoteUser);
}


extern "C" int
luaRequestGetContentLength(lua_State *lua) throw () {
    return call_method(lua, &Request::getUContentLength);
}


extern "C" int
luaRequestGetContentType(lua_State *lua) throw () {
    return call_method(lua, &Request::getContentType);
}

extern "C" int
luaRequestGetContentEncoding(lua_State *lua) throw () {
    return call_method(lua, &Request::getContentEncoding);
}

extern "C" int
luaRequestGetOriginalHost(lua_State *lua) throw () {
    return call_method(lua, &Request::getOriginalHost);
}

extern "C" int
luaRequestGetOriginalURI(lua_State *lua) throw () {
    return call_method(lua, &Request::getOriginalURI);
}

extern "C" int
luaRequestGetOriginalUrl(lua_State *lua) throw () {
    return call_method(lua, &Request::getOriginalUrl);
}



static const struct luaL_reg requestlib [] = {
    {"getArg",        luaRequestGetArg},
    {"getArgs",       luaRequestGetArgs},
    {"getHeader",     luaRequestGetHeader},
    {"getCookie",     luaRequestGetCookie},
    {"hasArg",        luaRequestHasArg},
    {"hasHeader",     luaRequestHasHeader},
    {"hasCookie",     luaRequestHasCookie},

    {"isSecure",        luaRequestIsSecure},
    {"isBot",           luaRequestIsBot},
    {"getURI",          luaRequestGetURI},
    {"getPath",         luaRequestGetPath},
    {"getQuery",        luaRequestGetQuery},
    {"getScriptName",   luaRequestGetScriptName},
    {"getMethod",       luaRequestGetMethod},
    {"getRemoteIp",     luaRequestGetRemoteIp},

    {"getHost",         luaRequestGetHost},
    {"getPathInfo",     luaRequestGetPathInfo},
    {"getRealPath",     luaRequestGetRealPath},
    {"getHTTPUser",     luaRequestGetHTTPUser},

    {"getContentLength",    luaRequestGetContentLength},
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
