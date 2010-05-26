#ifndef _XSCRIPT_LUA_XSCRIPT_METHODS_H_
#define _XSCRIPT_LUA_XSCRIPT_METHODS_H_

#include <lua.hpp>

namespace xscript {

class Block;
class Context;
class InvokeContext;

void setupXScript(lua_State *lua, std::string *buf);
InvokeContext* getInvokeContext(lua_State *lua);
Context* getContext(lua_State *lua);
Block* getBlock(lua_State *lua);

} // namespace xscript

#endif // _XSCRIPT_LUA_XSCRIPT_METHODS_H_

