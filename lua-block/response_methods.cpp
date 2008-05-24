#include "settings.h"

#include <stdexcept>
#include <boost/current_function.hpp>

#include "xscript/logger.h"
#include "xscript/response.h"

#include "stack.h"
#include "exception.h"
#include "method_map.h"
#include "response_methods.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

extern "C" int luaResponseSetStatus(lua_State *lua) throw ();
extern "C" int luaResponseSetHeader(lua_State *lua) throw ();
extern "C" int luaResponseSetCookie(lua_State *lua) throw ();
extern "C" int luaResponseRedirectToPath(lua_State *lua) throw ();
extern "C" int luaResponseSetContentType(lua_State *lua) throw ();

static const struct luaL_reg responselib [] = {
	{"setStatus",		luaResponseSetStatus},
	{"setHeader",		luaResponseSetHeader},
	{"setCookie",		luaResponseSetCookie},
	{"redirectToPath",	luaResponseRedirectToPath},
	{"setContentType",	luaResponseSetContentType},
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
		Response *resp = luaReadStack<Response>(lua, "xscript.response", 1);
		unsigned short status = luaReadStack<boost::uint32_t>(lua, 2);
		resp->setStatus(status);
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
		Response *resp = luaReadStack<Response>(lua, "xscript.response", 1);
		resp->setHeader(luaReadStack<std::string>(lua, 2), luaReadStack<std::string>(lua, 3));
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
luaResponseSetCookie(lua_State *lua) throw () {
	log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
	try {
		return 0;
	}
	catch (const std::exception &e) {
		return luaL_error(lua, "caught exception in state.setBool: %s", e.what());
	}
}

extern "C" int
luaResponseRedirectToPath(lua_State *lua) throw () {
	log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
	try {
		luaCheckStackSize(lua, 2);
		Response *resp = luaReadStack<Response>(lua, "xscript.response", 1);
		std::string value = luaReadStack<std::string>(lua, 2);
		resp->redirectToPath(value);
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
		Response *resp = luaReadStack<Response>(lua, "xscript.response", 1);
		std::string value = luaReadStack<std::string>(lua, 2);
		resp->setContentType(value);
		return 0;
	}
	catch (const LuaError &e) {
		return e.translate(lua);
	}
	catch (const std::exception &e) {
		return luaL_error(lua, "caught exception in response.setStatus: %s", e.what());
	}
}

} // namespace xscript
