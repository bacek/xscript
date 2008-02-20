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

namespace xscript
{

extern "C" int luaRequestHasArg(lua_State *state) throw ();
extern "C" int luaRequestHasHeader(lua_State *state) throw ();
extern "C" int luaRequestHasCookie(lua_State *state) throw ();

template<>
MethodDispatcher<Request>::MethodDispatcher()
{
	registerMethod("hasArg", &luaRequestHasArg);
	registerMethod("hasHeader", &luaRequestHasHeader);
	registerMethod("hasCookie", &luaRequestHasCookie);
}

static MethodDispatcher<Request> disp_;

extern "C" int
luaRequestIndex(lua_State *lua) throw () {
	log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
	try {
		luaCheckStackSize(lua, 2);
		luaCheckUserData(lua, "xscript.request", 1);
		std::string method = luaReadStack<std::string>(lua, 2);
		log()->debug("%s, calling request method: %s", BOOST_CURRENT_FUNCTION, method.c_str());
		lua_pushcfunction(lua, disp_.findMethod(method));
		return 1;
		
	}
	catch (const LuaError &e) {
		return e.translate(lua);
	}
}

template<typename Func> int
luaRequestGet(lua_State *lua, Func func) {
	try {
		luaCheckStackSize(lua, 2);
		Request *req = luaReadStack<Request>(lua, "xscript.args", 1);
		std::string key = luaReadStack<std::string>(lua, 2);
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

} // namespace xscript
