#include "settings.h"

#include <new>
#include <memory>
#include <stdexcept>
#include <boost/thread/tss.hpp>
#include <boost/current_function.hpp>

#include "xscript/util.h"
#include "xscript/resource_holder.h"
#include "xscript/logger.h"
#include "xscript/context.h"

#include "stack.h"
#include "lua_block.h"
#include "state_methods.h"
#include "request_methods.h"
#include "response_methods.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

template<>
inline void ResourceHolderTraits<lua_State*>::destroy(lua_State *state) {
    lua_close(state);
}

typedef ResourceHolder<lua_State*> LuaHolder;

LuaBlock::LuaBlock(const Extension *ext, Xml *owner, xmlNodePtr node) :
	Block(ext, owner, node), code_(NULL)
{
}

LuaBlock::~LuaBlock() {
}

void
LuaBlock::parse() {
	
	xmlNodePtr ptr = NULL;
	for (ptr = node()->children; NULL != ptr; ptr = ptr->next) {
		if (XML_CDATA_SECTION_NODE == ptr->type) {
			code_ = (const char*) ptr->content;
			break;
		}
	}
	if (NULL == code_) {
		code_ = XmlUtils::value(node());
	}
	if (NULL == code_) {
		throw std::runtime_error("empty lua node");
	}
	LuaHolder lua(luaL_newstate());
	int res = luaL_loadstring(lua.get(), code_);
	if (LUA_ERRSYNTAX == res) {
		throw std::runtime_error("bad lua code");
	}
	else if (LUA_ERRMEM == res) {
		throw std::bad_alloc();
	}
}

extern "C" void luaHook(lua_State * lua, lua_Debug *ar) {
	(void)lua;
	(void)ar;
};

void
LuaBlock::setupState(State *state, lua_State *lua) {
	log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));

	lua_newtable(lua);
	lua_setglobal(lua, "xscript");

	//lua_sethook(lua, luaHook, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE, 1);

	luaL_newmetatable(lua, "xscript.state");
	lua_pushstring(lua, "__index");
	lua_pushvalue(lua, -2);  /* pushes the metatable */
	lua_settable(lua, -3);  /* metatable.__index = metatable */
	
	luaL_openlib(lua, 0, getStatelib(), 0);
	luaL_openlib(lua, "xscript_state", getStatelib(), 0);
	
	// Get global xscript. We will set 'state' field later
	lua_getglobal(lua, "xscript");

    statePtr *p = (statePtr *)lua_newuserdata(lua, sizeof(statePtr));
	p->ptr = state;

	luaL_getmetatable(lua, "xscript.state");
	lua_setmetatable(lua, -2); // points to new userdata
	
	// Our userdata is on top of stack.
	// Assign it to 'state'
	lua_setfield(lua, -2, "state"); 

	// And remove it from stack
	lua_remove(lua, -1);

	log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
	return;
}


void
LuaBlock::setupRequest(Request *request, lua_State *lua) {
	log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));

	luaL_newmetatable(lua, "xscript.request");
	lua_pushstring(lua, "__index");
	lua_pushvalue(lua, -2);  /* pushes the metatable */
	lua_settable(lua, -3);  /* metatable.__index = metatable */
	
	luaL_openlib(lua, 0, getRequestLib(), 0);
	luaL_openlib(lua, "xscript_state", getRequestLib(), 0);
	
	// Get global xscript. We will set 'state' field later
	lua_getglobal(lua, "xscript");

    requestPtr *p = (requestPtr *)lua_newuserdata(lua, sizeof(requestPtr));
	p->ptr = request;

	luaL_getmetatable(lua, "xscript.request");
	lua_setmetatable(lua, -2); // points to new userdata
	
	// Our userdata is on top of stack.
	// Assign it to 'state'
	lua_setfield(lua, -2, "request"); 

	// And remove it from stack
	lua_remove(lua, -1);

	log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
	return;

	lua_pushstring(lua, "request");
	lua_pushlightuserdata(lua, request);
	if (0 == luaL_newmetatable(lua, "xscript.request")) {
		throw std::runtime_error("can not register xscript.request");
	}
	lua_pushstring(lua, "__index");
	lua_pushcfunction(lua, &luaRequestIndex);
	lua_rawset(lua, 3);
	if (0 == lua_setmetatable(lua, 2)) {
		throw std::runtime_error("failed to set request metatable");
	}
	log()->debug("%s, metatable for xscript.request set up", BOOST_CURRENT_FUNCTION);
	lua_rawset(lua, LUA_GLOBALSINDEX);
}

void
LuaBlock::setupResponse(Response *response, lua_State *lua) {
	log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));

	return;

	lua_pushstring(lua, "response");
	lua_pushlightuserdata(lua, response);
	if (0 == luaL_newmetatable(lua, "xscript.response")) {
		throw std::runtime_error("can not register xscript.response");
	}
	lua_pushstring(lua, "__index");
	lua_pushcfunction(lua, &luaResponseIndex);
	lua_rawset(lua, 3);
	if (0 == lua_setmetatable(lua, 2)) {
		throw std::runtime_error("failed to set response metatable");
	}
	log()->debug("%s, metatable for xscript.response set up", BOOST_CURRENT_FUNCTION);
	lua_rawset(lua, LUA_GLOBALSINDEX);
}

void
LuaBlock::setupRequestArgs(Request *req, lua_State *lua) {
	(void)req;
	(void)lua;
}

void
LuaBlock::setupRequestHeaders(Request *req, lua_State *lua) {
	(void)req;
	(void)lua;
}

void
LuaBlock::setupRequestCookies(Request *req, lua_State *lua) {
	(void)req;
	(void)lua;
}

XmlDocHelper
LuaBlock::call(Context *ctx, boost::any &) throw (std::exception) {
	
	log()->entering(BOOST_CURRENT_FUNCTION);
	
	LuaHolder lua(luaL_newstate());
	luaL_openlibs(lua.get());
		
	Request *request = ctx->request();
	boost::shared_ptr<State> state = ctx->state();
	
	setupState(state.get(), lua.get());
	setupRequest(request, lua.get());
	setupRequestArgs(request, lua.get());
	setupRequestHeaders(request, lua.get());
	setupRequestCookies(request, lua.get());
	
	if (LUA_ERRMEM == luaL_loadstring(lua.get(), code_)) {
		throw std::bad_alloc();
	}
	int res = lua_pcall(lua.get(), 0, LUA_MULTRET, 1);
	if (res != 0) {
		std::string msg = luaReadStack<std::string>(lua.get(), 1);
		log()->error("%s, Lua block failed: %s", BOOST_CURRENT_FUNCTION, msg.c_str());
		throw (msg);
	}
	
	return fakeResult();
}

LuaExtension::LuaExtension() {
}

LuaExtension::~LuaExtension() {
}

	
const char*
LuaExtension::name() const {
	return "lua";
}

const char*
LuaExtension::nsref() const {
	return XmlUtils::XSCRIPT_NAMESPACE;
}
	
void
LuaExtension::initContext(Context *ctx) {
	(void)ctx;
}

void
LuaExtension::stopContext(Context *ctx) {
	(void)ctx;
}

void
LuaExtension::destroyContext(Context *ctx) {
	(void)ctx;
}
	
std::auto_ptr<Block>
LuaExtension::createBlock(Xml *owner, xmlNodePtr node) {
	return std::auto_ptr<Block>(new LuaBlock(this, owner, node));
}

void
LuaExtension::init(const Config *config) {
	(void)config;
}

static ExtensionRegisterer reg_(ExtensionHolder(new LuaExtension()));

} // namespace xscript

extern "C" ExtensionInfo* get_extension_info() {
    static ExtensionInfo info = { "lua", xscript::XmlUtils::XSCRIPT_NAMESPACE };
    return &info;
}

