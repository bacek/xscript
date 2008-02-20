#ifndef _XSCRIPT_LUA_REQUEST_METHODS_H_
#define _XSCRIPT_LUA_REQUEST_METHODS_H_

#include <lua.hpp>

namespace xscript
{

extern "C" int luaRequestIndex(lua_State *lua) throw ();

extern "C" int luaRequestGetArg(lua_State *lua) throw ();

extern "C" int luaRequestGetHeader(lua_State *lua) throw ();

extern "C" int luaRequestGetCookie(lua_State *lua) throw ();

} // namespace xscript

#endif // _XSCRIPT_LUA_REQUEST_METHODS_H_
