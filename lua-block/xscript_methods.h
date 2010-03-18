#ifndef _XSCRIPT_LUA_XSCRIPT_METHODS_H_
#define _XSCRIPT_LUA_XSCRIPT_METHODS_H_

#include <lua.hpp>

namespace xscript {

class Block;
class Context;

void setupXScript(lua_State *lua, std::string *buf);
Context* getContext(lua_State *lua);
Block* getBlock(lua_State *lua);

} // namespace xscript

#endif // _XSCRIPT_LUA_XSCRIPT_METHODS_H_

