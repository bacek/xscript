#ifndef _XSCRIPT_LUA_STATE_METHODS_H_
#define _XSCRIPT_LUA_STATE_METHODS_H_

#include <lua.hpp>

namespace xscript
{

extern "C" int luaStateIndex(lua_State *lua) throw ();

const struct luaL_reg * getStatelib();

} // namespace xscript

#endif // _XSCRIPT_LUA_STATE_METHODS_H_
