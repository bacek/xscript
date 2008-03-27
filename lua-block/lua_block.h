#ifndef _XSCRIPT_LUA_BLOCK_H_
#define _XSCRIPT_LUA_BLOCK_H_

#include <lua.hpp>

#include "xscript/block.h"
#include "xscript/extension.h"

namespace xscript
{

class State;
class Request;
class Response;

class LuaBlock : public Block
{
public:
	LuaBlock(const Extension *ext, Xml *owner, xmlNodePtr node);
	virtual ~LuaBlock();
	virtual void parse();
	
protected:

	void setupErrorReport(lua_State *lua);
	void setupState(State *state, lua_State *lua);
	void setupRequest(Request *request, lua_State *lua);
	void setupResponse(Response *response, lua_State *lua);
	
	void setupRequestArgs(Request *request, lua_State *lua);
	void setupRequestHeaders(Request *request, lua_State *lua);
	void setupRequestCookies(Request *request, lua_State *lua);
	
	void reportError(const char *message, lua_State *lua);
	virtual XmlDocHelper call(Context *ctx, boost::any &) throw (std::exception);
	
private:
	const char *code_;
};

class LuaExtension : public Extension
{
public:
	LuaExtension();
	virtual ~LuaExtension();
	
	virtual const char* name() const;
	virtual const char* nsref() const;
	
	virtual void initContext(Context *ctx);
	virtual void stopContext(Context *ctx);
	virtual void destroyContext(Context *ctx);
	
	virtual std::auto_ptr<Block> createBlock(Xml *owner, xmlNodePtr node);
	virtual void init(const Config *config);

private:
	LuaExtension(const LuaExtension &);
	LuaExtension& operator = (const LuaExtension &);
};


} // namespace xscript

#endif // _XSCRIPT_LUA_BLOCK_H_
