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

static const struct luaL_reg statelib [] = {
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
    
const struct luaL_reg * getStatelib() {
	return statelib;
}


template<typename T>
struct pointer {
	T* ptr;
};
typedef pointer<State> statePtr;

State * getState(lua_State *lua) {
	void *ud = luaL_checkudata(lua, 1, "xscript.state");
	luaL_argcheck(lua, ud != NULL, 1, "`state' expected");
	return ((statePtr *)ud)->ptr;
}


extern "C" int
luaStateGet(lua_State *lua) throw () {
	log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
	try {
		State* s = getState(lua);
		std::string key = luaReadStack<std::string>(lua, 2);
		log()->debug("luaStateGet: %s", key.c_str());
		std::string value = s->asString(key);
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
		State* s = getState(lua);
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
