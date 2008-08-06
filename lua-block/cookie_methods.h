#ifndef _XSCRIPT_LUA_COOKIE_H
#define _XSCRIPT_LUA_COOKIE_H

#include <lua.hpp>

namespace xscript {

// Cookie is different from other objects. We are _really_ creates new
// cookies in lua. So method:
// const struct luaL_reg * getCookieLib();
// doesn't exists. We have next method instead:

void registerCookieMethods(lua_State *lua);

} // namespace xscript

#endif
