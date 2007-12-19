#ifndef _XSCRIPT_LUA_EXCEPTION_H_
#define _XSCRIPT_LUA_EXCEPTION_H_

#include <string>
#include <exception>
#include <lua.hpp>

namespace xscript
{

class LuaError : public std::exception
{
public:
	LuaError();
	virtual ~LuaError() throw ();
	virtual int translate(lua_State *lua) const throw () = 0;
};

class BadType : public LuaError 
{
public:
	BadType(const char *name, int index);
	virtual ~BadType() throw ();
	virtual const char* what() const throw ();
	virtual int translate(lua_State *lua) const throw ();
	
private:
	int index_;
	std::string name_;
};

class BadArgCount : public LuaError 
{
public:
	BadArgCount(int count);
	virtual ~BadArgCount() throw ();
	virtual const char* what() const throw ();
	virtual int translate(lua_State *lua) const throw ();
	
private:
	int count_;
};

} // namespace xscript

#endif // _XSCRIPT_LUA_EXCEPTION_H_
