#include "settings.h"

#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <boost/current_function.hpp>

#include "xscript/state.h"
#include "xscript/logger.h"

#include "stack.h"
#include "exception.h"
#include "method_map.h"
#include "state_methods.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

extern "C" int luaStateGet(lua_State *lua) throw ();
extern "C" int luaStateSetBool(lua_State *lua) throw ();

extern "C" int luaStateSetLong(lua_State *lua) throw ();
extern "C" int luaStateSetLongLong(lua_State *lua) throw ();

extern "C" int luaStateSetULong(lua_State *lua) throw ();
extern "C" int luaStateSetULongLong(lua_State *lua) throw ();

extern "C" int luaStateSetDouble(lua_State *lua) throw ();
extern "C" int luaStateSetString(lua_State *lua) throw ();

template<>
MethodDispatcher<State>::MethodDispatcher() {
	registerMethod("get", &luaStateGet);
	registerMethod("setBool", &luaStateSetBool);
	registerMethod("setLong", &luaStateSetLong);
	registerMethod("setLongLong", &luaStateSetLongLong);
	registerMethod("setULong", &luaStateSetULong);
	registerMethod("setULongLong", &luaStateSetULongLong);
	registerMethod("setString", &luaStateSetString);
	registerMethod("setDouble", &luaStateSetDouble);
}

static MethodDispatcher<State> disp_;


extern "C" int
luaStateIndex(lua_State *lua) throw () {
	log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
	try {
		luaCheckStackSize(lua, 2);
		luaCheckUserData(lua, "xscript.state", 1);
		std::string method = luaReadStack<std::string>(lua, 2);
		log()->debug("%s, calling state method: %s", BOOST_CURRENT_FUNCTION, method.c_str());
		lua_pushcfunction(lua, disp_.findMethod(method));
		return 1;
	}
	catch (const LuaError &e) {
		return e.translate(lua);
	}
	catch (const std::exception &e) {
		return luaL_error(lua, "caught exception in state index: %s", e.what());
	}
}

extern "C" int
luaStateGet(lua_State *lua) throw () {
	log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
	try {
		luaCheckStackSize(lua, 2);
		State *state = luaReadStack<State>(lua, "xscript.state", 1);
		std::string value = state->asString(luaReadStack<std::string>(lua, 2));
		lua_pushstring(lua, value.c_str());
		return 1;
	}
	catch (const LuaError &e) {
		return e.translate(lua);
	}
	catch (const std::exception &e) {
		luaL_error(lua, "caught exception in state.get: %s", e.what());
		return 0;
	}
}

template<typename Type> int
luaStateSet(lua_State *lua) throw () {
	try {
		luaCheckStackSize(lua, 3);
		State *state = luaReadStack<State>(lua, "xscript.state", 1);
		state->set(luaReadStack<std::string>(lua, 2), luaReadStack<Type>(lua, 3));
		return 0;
	}
	catch (const LuaError &e) {
		return e.translate(lua);
	}
	catch (const std::exception &e) {
		return luaL_error(lua, "caught exception in state.set: %s", e.what());
	}
}

extern "C" int
luaStateSetBool(lua_State *lua) throw () {
	return luaStateSet<bool>(lua);
}

extern "C" int
luaStateSetLong(lua_State *lua) throw () {
	return luaStateSet<boost::int32_t>(lua);
}

extern "C" int
luaStateSetLongLong(lua_State *lua) throw () {
	return luaStateSet<boost::int64_t>(lua);
}

extern "C" int
luaStateSetULong(lua_State *lua) throw () {
	return luaStateSet<boost::uint32_t>(lua);
}

extern "C" int
luaStateSetULongLong(lua_State *lua) throw () {
	return luaStateSet<boost::uint64_t>(lua);
}

extern "C" int
luaStateSetString(lua_State *lua) throw () {
	return luaStateSet<std::string>(lua);
}

extern "C" int
luaStateSetDouble(lua_State *lua) throw () {
	return luaStateSet<double>(lua);
}

} // namespace xscript
