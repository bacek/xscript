#include "settings.h"

#include <stdexcept>
#include <boost/bind.hpp>
#include <boost/current_function.hpp>

#include "xscript/logger.h"
#include "xscript/request.h"
#include "xscript/encoder.h"
#include "xscript/util.h"
#include "xscript/string_utils.h"
#include "xscript/xml_util.h"

#include "stack.h"
#include "exception.h"
#include "method_map.h"
#include "xscript_methods.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

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
        if (stack_size < 1 || stack_size > 2) 
            throw BadArgCount(stack_size);

        std::string value = luaReadStack<std::string>(lua, 1);
        std::string encoded;
        if (stack_size == 2) {
            std::string encoding = luaReadStack<std::string>(lua, 2);
            
            std::auto_ptr<Encoder> encoder = Encoder::createEscaping("utf-8", encoding.c_str());
            encoder->encode(createRange(value), encoded);
        }
        else {
            encoded = value;
        }

        lua_pushstring(lua, StringUtils::urlencode(encoded).c_str());
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
        std::string decoded;
        if (stack_size == 2) {
            std::string encoding = luaReadStack<std::string>(lua, 2);
            std::auto_ptr<Encoder> encoder = Encoder::createEscaping(encoding.c_str(), "utf-8");
            encoder->encode(StringUtils::urldecode(value), decoded);
        }
        else {
            decoded = StringUtils::urldecode(value);
        }

        lua_pushstring(lua, decoded.c_str());
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

        lua_pushstring(lua, StringUtils::parseDomainFromURL(url, level).c_str());
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

        std::string md5 = XmlUtils::escape(value.c_str());
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

    // Setup md5 function
    lua_pushcfunction(lua, &luaMD5);
    lua_setfield(lua, -2, "md5");

    lua_pushcfunction(lua, &luaDomain);
    lua_setfield(lua, -2, "domain");

    lua_pushcfunction(lua, &luaXmlEscape);
    lua_setfield(lua, -2, "xmlescape");

    lua_pop(lua, 2); // pop _G and xscript

    log()->debug("%s, <<<stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
}

}
