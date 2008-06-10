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

extern "C" int luaStateHas(lua_State *lua);
extern "C" int luaStateGet(lua_State *lua);

extern "C" int luaStateSetBool(lua_State *lua);
extern "C" int luaStateSetLong(lua_State *lua);
extern "C" int luaStateSetLongLong(lua_State *lua);
extern "C" int luaStateSetULong(lua_State *lua);
extern "C" int luaStateSetULongLong(lua_State *lua);
extern "C" int luaStateSetDouble(lua_State *lua);
extern "C" int luaStateSetString(lua_State *lua);

static const struct luaL_reg statelib [] = {
      {"has",			luaStateHas},
      {"get",			luaStateGet},
      {"setBool",		luaStateSetBool},
	  {"setLong",		luaStateSetLong},
	  {"setLongLong",	luaStateSetLongLong},
	  {"setULong",		luaStateSetULong},
	  {"setULongLong",	luaStateSetULongLong},
	  {"setString",		luaStateSetString},
	  {"setDouble",		luaStateSetDouble},
      {NULL, NULL}
    };
    
const struct luaL_reg * getStateLib() {
	return statelib;
}

extern "C" int
luaStateHas(lua_State *lua) {
	log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
	try {
		luaCheckStackSize(lua, 2);

		State* s = luaReadStack<State>(lua, "xscript.state", 1);
		std::string key = luaReadStack<std::string>(lua, 2);
		log()->debug("luaStateHas: %s", key.c_str());
		lua_pushboolean(lua, s->has(key));
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

extern "C" int
luaStateGet(lua_State *lua) {
	log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
	try {
		luaCheckStackSize(lua, 2);

		State* s = luaReadStack<State>(lua, "xscript.state", 1);
		std::string key = luaReadStack<std::string>(lua, 2);
		log()->debug("luaStateGet: %s", key.c_str());
		if (s->has(key)) {
			std::string value = s->asString(key);
			lua_pushstring(lua, value.c_str());
		}
		else {
			lua_pushstring(lua, "");
		}
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
luaStateSet(lua_State *lua) {
	try {
		luaCheckStackSize(lua, 3);

		State* s = luaReadStack<State>(lua, "xscript.state", 1);
		std::string key = luaReadStack<std::string>(lua, 2);
		Type value = luaReadStack<Type>(lua, 3);
		log()->debug("luaStateSet: %s", key.c_str());
		s->set(key, value);
		return 0;
	}
	catch (const LuaError &e) {
		return e.translate(lua);
	}
	catch (const std::exception &e) {
		log()->debug("caught exception in state.set: %s", e.what());
		return luaL_error(lua, "caught exception in state.set: %s", e.what());
	}
}

extern "C" int
luaStateSetBool(lua_State *lua) {
	return luaStateSet<bool>(lua);
}

extern "C" int
luaStateSetLong(lua_State *lua) {
	return luaStateSet<boost::int32_t>(lua);
}

extern "C" int
luaStateSetLongLong(lua_State *lua) {
	return luaStateSet<boost::int64_t>(lua);
}

extern "C" int
luaStateSetULong(lua_State *lua) {
	return luaStateSet<boost::uint32_t>(lua);
}

extern "C" int
luaStateSetULongLong(lua_State *lua) {
	return luaStateSet<boost::uint64_t>(lua);
}

extern "C" int
luaStateSetString(lua_State *lua) {
	return luaStateSet<std::string>(lua);
}

extern "C" int
luaStateSetDouble(lua_State *lua) {
	return luaStateSet<double>(lua);
}

} // namespace xscript
