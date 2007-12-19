#include "settings.h"
#include "exception.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

LuaError::LuaError()
{
}

LuaError::~LuaError() throw () {
}

BadType::BadType(const char *name, int index) : 
	index_(index), name_(name)
{
}

BadType::~BadType() throw () {
	
}

const char*
BadType::what() const throw () {
	return "bad argument type";
}

int
BadType::translate(lua_State *lua) const throw () {
	return luaL_typerror(lua, index_, name_.c_str());
}

BadArgCount::BadArgCount(int count) : 
	count_(count)
{
}

BadArgCount::~BadArgCount() throw () {
}

const char*
BadArgCount::what() const throw () {
	return "bad arguments count";
}

int
BadArgCount::translate(lua_State *lua) const throw () {
	return luaL_argerror(lua, count_, "");
}

} // namespace xscript
