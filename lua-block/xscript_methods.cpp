#include "settings.h"

#include <strings.h>
#include <stdexcept>
#include <boost/bind.hpp>
#include <boost/current_function.hpp>

#include "xscript/context.h"
#include "xscript/encoder.h"
#include "xscript/http_utils.h"
#include "xscript/logger.h"
#include "xscript/request.h"
#include "xscript/punycode_utils.h"
#include "xscript/string_utils.h"
#include "xscript/util.h"
#include "xscript/xml_util.h"

#include "internal/vhost_arg_param.h"

#include "stack.h"
#include "exception.h"
#include "lua_block.h"
#include "method_map.h"
#include "xscript_methods.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

Context* getContext(lua_State *lua) {
    lua_getglobal(lua, "xscript");
    lua_getfield(lua, -1, "_ctx");

    pointer<Context> *p = (pointer<Context>*)lua_touserdata(lua, -1);
    assert(p);
    Context* ctx = p->ptr;

    lua_pop(lua, 2);
    
    return ctx;
}

InvokeContext* getInvokeContext(lua_State *lua) {
    lua_getglobal(lua, "xscript");
    lua_getfield(lua, -1, "_invoke_ctx");

    pointer<InvokeContext> *p = (pointer<InvokeContext>*)lua_touserdata(lua, -1);
    assert(p);
    InvokeContext* ctx = p->ptr;

    lua_pop(lua, 2);

    return ctx;
}

const LuaBlock* getBlock(lua_State *lua) {
    lua_getglobal(lua, "xscript");
    lua_getfield(lua, -1, "_block");

    pointer<const LuaBlock> *p = (pointer<const LuaBlock>*)lua_touserdata(lua, -1);
    assert(p);
    const LuaBlock* block = p->ptr;

    lua_pop(lua, 2);
    
    return block;
}

static int luaPrint (lua_State *lua) {
    try {
        int n = lua_gettop(lua);  /* number of arguments */
        int i;
    
        log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
        lua_getglobal(lua, "xscript");
        lua_getfield(lua, -1, "_buf");
    
        pointer<std::string> * p = (pointer<std::string>*)lua_touserdata(lua, -1);
        assert(p);
        std::string* buf = p->ptr;
        assert(buf);
    
        lua_pop(lua, 2); // pop xscript and buf
        log()->debug("%s, stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    
        //std::string* buf = output_buffer_.get();
    
        lua_getglobal(lua, "tostring");
        std::string local_buf; 
        for (i=1; i<=n; i++) {
            const char *s;
            lua_pushvalue(lua, -1);  /* function to be called */
            lua_pushvalue(lua, i);   /* value to print */
            lua_call(lua, 1, 1);
            s = lua_tostring(lua, -1);  /* get result */
            if (s == NULL)
                return luaL_error(lua, LUA_QL("tostring") " must return a string to "
                                  LUA_QL("print"));
            if (i>1) local_buf.push_back('\t');
            local_buf.append(s);
            lua_pop(lua, 1);  /* pop result */
        }
        if (local_buf.empty()) {
            buf->push_back('\n');
        }
        else {
            if (!buf->empty()) buf->push_back('\n');
            buf->append(local_buf);
        }
    }
    catch (const std::exception &e) {
        log()->error("caught exception in [xscript:print]: %s", e.what());
        luaL_error(lua, e.what());
    }
    return 0;
}

static int
luaUrlEncode(lua_State *lua) {
    try {
        int stack_size = lua_gettop(lua);
        if (stack_size < 1 || stack_size > 3)
            throw BadArgCount(stack_size);

        std::string value = luaReadStack<std::string>(lua, 1);
        if (!value.empty()) {
            if (stack_size >= 2) {
                std::string encoding = luaReadStack<std::string>(lua, 2);
                if (strcasecmp(encoding.c_str(), "utf-8")) {
                    std::auto_ptr<Encoder> encoder = Encoder::createEscaping("utf-8", encoding.c_str());
                    value = encoder->encode(createRange(value));
                }
            }
            bool force_percent = false;
            if (stack_size == 3) {
                force_percent = luaReadStack<bool>(lua, 3);
            }
            value = StringUtils::urlencode(value, force_percent);
        }

        lua_pushstring(lua, value.c_str());
        // Our value on stack
        return 1;
    }
    catch (const std::exception &e) {
        log()->error("caught exception in [xscript:urlencode]: %s", e.what());
        luaL_error(lua, e.what());
    }
    return 0;
}

static int
luaUrlDecode(lua_State *lua) {
    try {
        int stack_size = lua_gettop(lua);
        if (stack_size < 1 || stack_size > 2) 
            throw BadArgCount(stack_size);

        std::string value = luaReadStack<std::string>(lua, 1);
        if (!value.empty()) {
            value = StringUtils::urldecode(value);
            if (stack_size == 2) {
                std::string encoding = luaReadStack<std::string>(lua, 2);
                if (strcasecmp(encoding.c_str(), "utf-8")) {
                    std::auto_ptr<Encoder> encoder = Encoder::createEscaping(encoding.c_str(), "utf-8");
                    value = encoder->encode(StringUtils::urldecode(value));
                }
            }
        }

        lua_pushstring(lua, value.c_str());
        // Our value on stack
        return 1;
    }
    catch (const std::exception &e) {
        log()->error("caught exception in [xscript:urldecode]: %s", e.what());
        luaL_error(lua, e.what());
    }
    return 0;
}

static int
luaPunycodeDomainEncode(lua_State *lua) {
    try {
        int stack_size = lua_gettop(lua);
        if (stack_size < 1 || stack_size > 2) 
            throw BadArgCount(stack_size);

        std::string value = luaReadStack<std::string>(lua, 1);
        std::string encoding;
        if (stack_size == 2) {
            encoding = luaReadStack<std::string>(lua, 2);
        }

        std::string result;
        if (!value.empty()) {
            PunycodeEncoder encoder(encoding.c_str());
            encoder.domainEncode(createRange(value), result);
        }

        lua_pushstring(lua, result.c_str());
        // Our value on stack
        return 1;
    }
    catch (const std::exception &e) {
        log()->error("caught exception in [xscript:punycodeDomainEncode]: %s", e.what());
        luaL_error(lua, e.what());
    }
    return 0;
}

static int
luaPunycodeDomainDecode(lua_State *lua) {
    try {
        int stack_size = lua_gettop(lua);
        if (stack_size < 1 || stack_size > 2) 
            throw BadArgCount(stack_size);

        std::string value = luaReadStack<std::string>(lua, 1);
        std::string encoding;
        if (stack_size == 2) {
            encoding = luaReadStack<std::string>(lua, 2);
        }

        std::string result;
        if (!value.empty()) {
            PunycodeDecoder decoder(encoding.c_str());
            decoder.domainDecode(createRange(value), result);
        }

        lua_pushstring(lua, result.c_str());
        // Our value on stack
        return 1;
    }
    catch (const std::exception &e) {
        log()->error("caught exception in [xscript:punycodeDomainDecode]: %s", e.what());
        luaL_error(lua, e.what());
    }
    return 0;
}

static int
luaMD5(lua_State *lua) {
    try {
        luaCheckStackSize(lua, 1);
        std::string value = luaReadStack<std::string>(lua, 1);

        std::string md5 = HashUtils::hexMD5(value.c_str());
        lua_pushstring(lua, md5.c_str());
        // Our value on stack
        return 1;
    }
    catch (const std::exception &e) {
        log()->error("caught exception in [xscript:md5]: %s", e.what());
        luaL_error(lua, e.what());
    }
    return 0;
}

static int
luaBase64Encode(lua_State *lua) {
    try {
        luaCheckStackSize(lua, 1);
        std::string data = luaReadStack<std::string>(lua, 1);

        HashUtils::encodeBase64(data.c_str(), data.size(), data);
        lua_pushstring(lua, data.c_str());
        // Our value on stack
        return 1;
    }
    catch (const std::exception &e) {
        log()->error("caught exception in [xscript:base64encode]: %s", e.what());
        luaL_error(lua, e.what());
    }
    return 0;
}

static int
luaBase64Decode(lua_State *lua) {
    try {
        luaCheckStackSize(lua, 1);
        std::string data = luaReadStack<std::string>(lua, 1);

        HashUtils::decodeBase64(data.c_str(), data.size(), data);
        lua_pushstring(lua, data.c_str());
        // Our value on stack
        return 1;
    }
    catch (const std::exception &e) {
        log()->error("caught exception in [xscript:base64decode]: %s", e.what());
        luaL_error(lua, e.what());
    }
    return 0;
}

static int
luaDomain(lua_State *lua) {
    try {
        int stack_size = lua_gettop(lua);
        if (stack_size < 1 || stack_size > 2) 
            throw BadArgCount(stack_size);

        std::string url = luaReadStack<std::string>(lua, 1);
        boost::int32_t level = 0;
        if (stack_size == 2) {
            level = luaReadStack<boost::int32_t>(lua, 2);
        }

        try {
            lua_pushstring(lua, StringUtils::parseDomainFromURL(url, level).c_str());
        }
        catch (const std::exception &) {
            lua_pushnil(lua);
        }
        // Our value on stack
        return 1;
    }
    catch (const std::exception &e) {
        log()->error("caught exception in [xscript:domain]: %s", e.what());
        luaL_error(lua, e.what());
    }
    return 0;
}

static int
luaXmlEscape(lua_State *lua) {
    try {
        luaCheckStackSize(lua, 1);
        std::string value = luaReadStack<std::string>(lua, 1);

        std::string escaped = XmlUtils::escape(value);
        lua_pushstring(lua, escaped.c_str());
        // Our value on stack
        return 1;
    }
    catch (const std::exception &e) {
        log()->error("caught exception in [xscript:xmlescape]: %s", e.what());
        luaL_error(lua, e.what());
    }
    return 0;
}

static int
luaAttachStylesheet(lua_State *lua) {   
    try {              
        luaCheckStackSize(lua, 1);
        std::string xslt = luaReadStack<std::string>(lua, 1);
        
        Context *ctx = getContext(lua);
        if (NULL == ctx) {
            throw std::runtime_error("Undefined context");
        }
        xslt = getBlock(lua)->fullName(xslt);
        ctx->originalContext()->xsltName(xslt);
    }
    catch (const std::exception &e) {
        log()->error("caught exception in [xscript:attachStylesheet]: %s", e.what());
        luaL_error(lua, e.what());
    }
    return 0;
}

static int
luaDropStylesheet(lua_State *lua) {   
    try {              
        luaCheckStackSize(lua, 0);
        
        Context *ctx = getContext(lua);
        if (NULL == ctx) {
            throw std::runtime_error("Undefined context");
        }
        ctx->originalContext()->xsltName(StringUtils::EMPTY_STRING);
    }
    catch (const std::exception &e) {
        log()->error("caught exception in [xscript:dropStylesheet]: %s", e.what());
        luaL_error(lua, e.what());
    }
    return 0;
}

static int
luaSuppressBody(lua_State *lua) {   
    try {              
        luaCheckStackSize(lua, 0);
        
        Context *ctx = getContext(lua);
        if (NULL == ctx) {
            throw std::runtime_error("Undefined context");
        }
        ctx->response()->setSuppressBody(true);
    }
    catch (const std::exception &e) {
        log()->error("caught exception in [xscript:suppressBody]: %s", e.what());
        luaL_error(lua, e.what());
    }
    return 0;
}

static int
luaSkipNextBlocks(lua_State *lua) {   
    try {              
        luaCheckStackSize(lua, 0);
        
        Context *ctx = getContext(lua);
        if (NULL == ctx) {
            throw std::runtime_error("Undefined context");
        }
        ctx->skipNextBlocks(true);
    }
    catch (const std::exception &e) {
        log()->error("caught exception in [xscript:skipNextBlocks]: %s", e.what());
        luaL_error(lua, e.what());
    }
    return 0;
}

static int
luaStopBlocks(lua_State *lua) {   
    try {              
        luaCheckStackSize(lua, 0);
        
        Context *ctx = getContext(lua);
        if (NULL == ctx) {
            throw std::runtime_error("Undefined context");
        }
        ctx->stopBlocks(true);
    }
    catch (const std::exception &e) {
        log()->error("caught exception in [xscript:stopBlocks]: %s", e.what());
        luaL_error(lua, e.what());
    }
    return 0;
}

static int
luaGetVHostArg(lua_State *lua) {
    try {
        luaCheckStackSize(lua, 1);
        std::string name = luaReadStack<std::string>(lua, 1);
        
        Context *ctx = getContext(lua);
        lua_pushstring(lua, VHostArgParam::variable(ctx, name).c_str());
        // Our value on stack
        return 1;
    }
    catch (const std::exception &e) {
        log()->error("caught exception in [xscript:getVHostArg]: %s", e.what());
        luaL_error(lua, e.what());
    }
    return 0;
}

static int
luaGetXmlBase(lua_State *lua) {
    try {
        luaCheckStackSize(lua, 0);
        lua_pushstring(lua, getBlock(lua)->getBase().c_str());
        // Our value on stack
        return 1;
    }
    catch (const std::exception &e) {
        log()->error("caught exception in [xscript:getXmlBase]: %s", e.what());
        luaL_error(lua, e.what());
    }
    return 0;
}

static int
luaStrSplit(lua_State *lua) {
    try {
        luaCheckStackSize(lua, 2);
        std::string data = luaReadStack<std::string>(lua, 1);
        std::string delim = luaReadStack<std::string>(lua, 2);

        std::auto_ptr<std::vector<std::string> > result(new std::vector<std::string>());
        StringUtils::split(data, delim, *result);
        luaPushStack(lua, result);
        // Our value on stack
        return 1;
    }
    catch (const std::exception &e) {
        log()->error("caught exception in [xscript:strsplit]: %s", e.what());
        luaL_error(lua, e.what());
    }
    return 0;
}

static int
luaSetExpireDelta(lua_State *lua) throw () {
    try {
        luaCheckStackSize(lua, 1);
        Context *ctx = getContext(lua);
        boost::int32_t expire_time_delta = luaReadStack<boost::int32_t>(lua, 1);
        if (expire_time_delta < 0) {
            throw std::runtime_error("negative expire time delta is not allowed");
        }
        ctx->setExpireDelta((boost::uint32_t)expire_time_delta);
        return 0;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in [xscript:setExpireDelta]: %s", e.what());
    }
}

static int
luaDateParse(lua_State *lua) throw () {
    try {
        luaCheckStackSize(lua, 1);
        std::string date = luaReadStack<std::string>(lua, 1);
        lua_pushinteger(lua, HttpDateUtils::parse(date.c_str()));
        return 1;
    }
    catch (const LuaError &e) {
        return e.translate(lua);
    }
    catch (const std::exception &e) {
        return luaL_error(lua, "caught exception in [xscript:dateparse]: %s", e.what());
    }
}

void
setupXScript(lua_State *lua, std::string * buf) {
    log()->debug("%s, >>>stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));

    lua_newtable(lua);
    lua_setglobal(lua, "xscript");

    lua_getglobal(lua, "_G");
    lua_pushcfunction(lua, &luaPrint);
    lua_setfield(lua, -2, "print");

    lua_getglobal(lua, "xscript");

    pointer<std::string> *p = (pointer<std::string> *)lua_newuserdata(lua, sizeof(pointer<std::string>));
    p->ptr = buf;

    // Our userdata is on top of stack.
    // Assign it to '_buf'
    lua_setfield(lua, -2, "_buf");
    
    // Setup urlencode and urldecode
    lua_pushcfunction(lua, &luaUrlEncode);
    lua_setfield(lua, -2, "urlencode");
    lua_pushcfunction(lua, &luaUrlDecode);
    lua_setfield(lua, -2, "urldecode");

    // Setup punycode domain encode and decode
    lua_pushcfunction(lua, &luaPunycodeDomainEncode);
    lua_setfield(lua, -2, "punycodeDomainEncode");
    lua_pushcfunction(lua, &luaPunycodeDomainDecode);
    lua_setfield(lua, -2, "punycodeDomainDecode");

    // Setup md5 function
    lua_pushcfunction(lua, &luaMD5);
    lua_setfield(lua, -2, "md5");

    lua_pushcfunction(lua, &luaBase64Encode);
    lua_setfield(lua, -2, "base64encode");
    
    lua_pushcfunction(lua, &luaBase64Decode);
    lua_setfield(lua, -2, "base64decode");
    
    lua_pushcfunction(lua, &luaDomain);
    lua_setfield(lua, -2, "domain");

    lua_pushcfunction(lua, &luaXmlEscape);
    lua_setfield(lua, -2, "xmlescape");
    
    lua_pushcfunction(lua, &luaAttachStylesheet);
    lua_setfield(lua, -2, "attachStylesheet");
    
    lua_pushcfunction(lua, &luaDropStylesheet);
    lua_setfield(lua, -2, "dropStylesheet");
    
    lua_pushcfunction(lua, &luaSuppressBody);
    lua_setfield(lua, -2, "suppressBody");

    lua_pushcfunction(lua, &luaSkipNextBlocks);
    lua_setfield(lua, -2, "skipNextBlocks");
    
    lua_pushcfunction(lua, &luaStopBlocks);
    lua_setfield(lua, -2, "stopBlocks");
    
    lua_pushcfunction(lua, &luaGetVHostArg);
    lua_setfield(lua, -2, "getVHostArg");
    
    lua_pushcfunction(lua, &luaStrSplit);
    lua_setfield(lua, -2, "strsplit");
    
    lua_pushcfunction(lua, &luaSetExpireDelta);
    lua_setfield(lua, -2, "setExpireDelta");
    
    lua_pushcfunction(lua, &luaDateParse);
    lua_setfield(lua, -2, "dateparse");

    lua_pushcfunction(lua, &luaGetXmlBase);
    lua_setfield(lua, -2, "getXmlBase");

    lua_pop(lua, 2); // pop _G and xscript

    log()->debug("%s, <<<stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
}

}
