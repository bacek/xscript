#ifndef _XSCRIPT_LUA_META_METHODS_H_
#define _XSCRIPT_LUA_META_METHODS_H_

#include <lua.hpp>

namespace xscript {

const struct luaL_reg * getMetaLib();
const struct luaL_reg * getSelfMetaLib();

} // namespace xscript

#endif // _XSCRIPT_LUA_META_METHODS_H_
