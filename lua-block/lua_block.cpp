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

#include "lua_block.h"
#include "state_methods.h"
#include "request_methods.h"
#include "response_methods.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

static boost::thread_specific_ptr<std::string> error_message_;

template<>
inline void ResourceHolderTraits<lua_State*>::destroy(lua_State *state) {
    lua_close(state);
}

typedef ResourceHolder<lua_State*> LuaHolder;

extern "C" int
luaReportError(lua_State *lua) throw () {
	try {
		if (!lua_isstring(lua, 1)) {
			log()->error("failed to report lua error: can not get error message");
			return 0;
		}
		const char *val = lua_tostring(lua, 1);
		std::string *msg = error_message_.get();
		assert(NULL != msg);
		msg->assign(val);
	}
	catch (const std::exception &e) {
		log()->error("failed to report lua error: %s", e.what());
	}
	return 0;
}

LuaBlock::LuaBlock(Xml *owner, xmlNodePtr node) :
	Block(owner, node), code_(NULL)
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

void
LuaBlock::setupErrorReport(lua_State *lua) {
	std::string *msg = error_message_.get();
	if (NULL == msg) {
		std::auto_ptr<std::string> ptr = std::auto_ptr<std::string>(new std::string());
		error_message_.reset(ptr.get());
		ptr.release();
	}
	else {
		msg->clear();
	}
	lua_pushcfunction(lua, &luaReportError);
}

void
LuaBlock::setupState(State *state, lua_State *lua) {
	log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
	lua_pushstring(lua, "state");
	lua_pushlightuserdata(lua, state);
	if (0 == luaL_newmetatable(lua, "xscript.state")) {
		throw std::runtime_error("can not register xscript.state");
	}
	lua_pushstring(lua, "__index");
	lua_pushcfunction(lua, &luaStateIndex);
	lua_rawset(lua, 3);
	if (0 == lua_setmetatable(lua, 2)) {
		throw std::runtime_error("failed to set state metatable");
	}
	log()->debug("%s, metatable for xscript.state set up", BOOST_CURRENT_FUNCTION);
	lua_rawset(lua, LUA_GLOBALSINDEX);
}

void
LuaBlock::setupResponse(Response *response, lua_State *lua) {
	log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
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
}

void
LuaBlock::setupRequestHeaders(Request *req, lua_State *lua) {
}

void
LuaBlock::setupRequestCookies(Request *req, lua_State *lua) {
}

XmlDocHelper
LuaBlock::call(Context *ctx, boost::any &) throw (std::exception) {
	
	log()->entering(BOOST_CURRENT_FUNCTION);
	
	LuaHolder lua(luaL_newstate());
	luaL_openlibs(lua.get());
		
	Request *request = ctx->request();
	boost::shared_ptr<State> state = ctx->state();
	
	setupState(state.get(), lua.get());
	setupRequestArgs(request, lua.get());
	setupRequestHeaders(request, lua.get());
	setupRequestCookies(request, lua.get());
	
	setupErrorReport(lua.get());
	if (LUA_ERRMEM == luaL_loadstring(lua.get(), code_)) {
		throw std::bad_alloc();
	}
	lua_pcall(lua.get(), 0, LUA_MULTRET, 1);
	
	std::string *msg = error_message_.get();
	assert(NULL != msg);
	
	if (!msg->empty()) {
		throw std::runtime_error(*msg);
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
}

void
LuaExtension::stopContext(Context *ctx) {
}

void
LuaExtension::destroyContext(Context *ctx) {
}
	
std::auto_ptr<Block>
LuaExtension::createBlock(Xml *owner, xmlNodePtr node) {
	return std::auto_ptr<Block>(new LuaBlock(owner, node));
}

void
LuaExtension::init(const Config *config) {
}

static ExtensionRegisterer reg_(ExtensionHolder(new LuaExtension()));

} // namespace xscript
