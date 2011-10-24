#include "settings.h"

#include <stdexcept>
#include <boost/current_function.hpp>

#include "xscript/context.h"
#include "xscript/logger.h"
#include "xscript/message_interface.h"
#include "xscript/response.h"

#include "stack.h"
#include "exception.h"
#include "lua_block.h"
#include "method_map.h"
#include "xscript_methods.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

extern "C" int luaResponseSetStatus(lua_State *lua) throw ();
extern "C" int luaResponseSetHeader(lua_State *lua) throw ();
extern "C" int luaResponseSetCookie(lua_State *lua) throw ();
extern "C" int luaResponseRedirectToPath(lua_State *lua) throw ();
extern "C" int luaResponseSetContentType(lua_State *lua) throw ();
extern "C" int luaResponseSetExpireTimeDelta(lua_State *lua) throw ();
extern "C" int luaResponseWrite(lua_State *lua) throw ();

static const struct luaL_reg responselib [] = {
    {"setStatus",       luaResponseSetStatus},
    {"setHeader",       luaResponseSetHeader},
    {"setCookie",       luaResponseSetCookie},
    {"redirectToPath",  luaResponseRedirectToPath},
    {"setContentType",  luaResponseSetContentType},
    {"setExpireTimeDelta",  luaResponseSetExpireTimeDelta},
    {"write", luaResponseWrite},
    {NULL, NULL}
};

extern "C" int
luaResponseSetStatus(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 2);
        luaReadStack<void>(lua, "xscript.response", 1);
        Response *response = getContext(lua)->response();
        unsigned short status = luaReadStack<boost::uint32_t>(lua, 2);
        response->setStatus(status);
        return 0;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in response.setStatus: %s", e.what());
    }
}

extern "C" int
luaResponseSetHeader(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 3);
        luaReadStack<void>(lua, "xscript.response", 1);
        Response *response = getContext(lua)->response();
        response->setHeader(
            luaReadStack<std::string>(lua, 2), luaReadStack<std::string>(lua, 3));
        return 0;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in response.setHeader: %s", e.what());
    }
}

extern "C" int
luaResponseRedirectToPath(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 2);
        luaReadStack<void>(lua, "xscript.response", 1);
        Response *response = getContext(lua)->response();
        std::string value = luaReadStack<std::string>(lua, 2);
        response->redirectToPath(value);
        return 0;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in response.redirectToPath: %s", e.what());
    }
}

extern "C" int
luaResponseSetContentType(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 2);
        luaReadStack<void>(lua, "xscript.response", 1);
        Response *response = getContext(lua)->response();
        std::string value = luaReadStack<std::string>(lua, 2);
        response->setContentType(value);
        return 0;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in response.setContentType: %s", e.what());
    }
}

extern "C" int
luaResponseSetCookie(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 2);
        luaReadStack<void>(lua, "xscript.response", 1);
        Response *response = getContext(lua)->response();
        Cookie * c = luaReadStack<Cookie>(lua, "xscript.cookie", 2);
        response->setCookie(*c);
        return 0;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in response.setCookie: %s", e.what());
    }
    return 0;
}

extern "C" int
luaResponseSetExpireTimeDelta(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 2);
        luaReadStack<void>(lua, "xscript.response", 1);
        Response *response = getContext(lua)->response();
        boost::int32_t expire_time_delta = luaReadStack<boost::int32_t>(lua, 2);
        if (expire_time_delta < 0) {
            throw std::runtime_error("negative expire time delta is not allowed");
        }
        response->setExpireDelta((boost::uint32_t)expire_time_delta);
        return 0;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in response.setExpireTimeDelta: %s", e.what());
    }
}

extern "C" int
luaResponseWrite(lua_State *lua) throw () {
    log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    try {
        luaCheckStackSize(lua, 2);
        luaReadStack<void>(lua, "xscript.response", 1);
        Context *ctx = getContext(lua);
        Response *response = ctx->response();
        std::string value = luaReadStack<std::string>(lua, 2);
        bool result = false;
        try {
            result = response->writeBinaryChunk(value.c_str(), value.size(), *ctx);
        }
        catch (const std::exception &e) {
	    // suppress write error
	    log()->warn("caught exception in lua response.write: %s", e.what());
        }
        log()->debug("%s, write size: %u, status: %d", BOOST_CURRENT_FUNCTION, value.size(), result);
        lua_pushboolean(lua, result);
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in response.write: %s", e.what());
    }
}

class LuaResponseRegisterHandler : public MessageHandler {
    Result process(const MessageParams &params, MessageResultBase &result) {
        (void)result;
        std::vector<LuaExtension::LuaRegisterFunc>* registerers =
                params.getPtr<std::vector<LuaExtension::LuaRegisterFunc> >(0);
        if (registerers) {
            registerers->push_back(boost::bind(&LuaExtension::registerLib,
                    _1, "response", true, responselib, responselib));
        }
        return CONTINUE;
    }
};

class LuaResponseHandlerRegisterer {
public:
    LuaResponseHandlerRegisterer() {
        MessageProcessor::instance()->registerBack("REGISTER_LUA_EXTENSION",
            boost::shared_ptr<MessageHandler>(new LuaResponseRegisterHandler()));
    }
};

static LuaResponseHandlerRegisterer reg_lua_response_handlers;

} // namespace xscript
