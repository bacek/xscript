#include "settings.h"

#include <stdexcept>
#include <boost/bind.hpp>
#include <boost/current_function.hpp>

#include "xscript/logger.h"
#include "xscript/request.h"

#include "stack.h"
#include "exception.h"
#include "request_methods.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

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
luaRequestGetArg(lua_State *lua) {
	log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
	return luaRequestGet(lua, boost::bind(&Request::getArg, _1, _2));
}

extern "C" int
luaRequestGetHeader(lua_State *lua) {
	log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
	return luaRequestGet(lua, boost::bind(&Request::getHeader, _1, _2));
}

extern "C" int
luaRequestGetCookie(lua_State *lua) {
	log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
	return luaRequestGet(lua, boost::bind(&Request::getCookie, _1, _2));
}

} // namespace xscript
