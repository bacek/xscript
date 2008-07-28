#include "settings.h"

#include <new>
#include <memory>
#include <stdexcept>
#include <boost/current_function.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>

#include "xscript/util.h"
#include "xscript/resource_holder.h"
#include "xscript/logger.h"
#include "xscript/context.h"
#include "xscript/encoder.h"

#include "stack.h"
#include "lua_block.h"
#include "state_methods.h"
#include "request_methods.h"
#include "response_methods.h"
#include "cookie_methods.h"

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

// Struct to store lua_State and mutex in Context
struct LuaState {
    LuaHolder       lua;
    boost::mutex    mutex;

    LuaState(lua_State * l) : lua(l) {
    };
};

typedef boost::shared_ptr<LuaState> LuaSharedContext;

static int
luaReportError(lua_State * lua) {

    if (lua_isstring(lua, 1)) { 
        const char *val = lua_tostring(lua, 1); 
        log()->error("lua block failed: %s", val);
    }
    else {
        log()->error("failed to report lua error: can not get error message"); 
    } 

    return 0;
}


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


/**
 * Setup userdata for lua.
 * We create lua's userdata with single poiner to our data inside, attach metatable
 * with methods and attach it as field to global 'xscript' variable with particular
 * name.
 * E.g. xscript::State::someMethod will be available as xscript.state:someMethod.
 */
template<typename Type>
void
setupUserdata(lua_State *lua, Type * type, const char* name, const struct luaL_reg * lib) {
    log()->debug("%s, >>>stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));

    //lua_sethook(lua, luaHook, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE, 1);

    std::string tableName = "xscript.";
    tableName += name;

    luaL_newmetatable(lua, tableName.c_str());
    lua_pushstring(lua, "__index");
    lua_pushvalue(lua, -2);  /* pushes the metatable */
    lua_settable(lua, -3);  /* metatable.__index = metatable */
    
    luaL_openlib(lua, 0, lib, 0);
    luaL_openlib(lua, tableName.c_str(), lib, 0);
    
    // Get global xscript. We will set 'state' field later
    lua_getglobal(lua, "xscript");

    pointer<Type> *p = (pointer<Type> *)lua_newuserdata(lua, sizeof(pointer<Type>));
    p->ptr = type;

    luaL_getmetatable(lua, tableName.c_str());
    lua_setmetatable(lua, -2); // points to new userdata
    
    // Our userdata is on top of stack.
    // Assign it to 'state'
    lua_setfield(lua, -2, name); 

    // And remove it from stack
    lua_remove(lua, -1);
    lua_pop(lua, 2); // pop xscript, __metatable

    log()->debug("%s, <<<stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
    return;
};


static int luaPrint (lua_State *lua) {
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
    for (i=1; i<=n; i++) {
        const char *s;
        lua_pushvalue(lua, -1);  /* function to be called */
        lua_pushvalue(lua, i);   /* value to print */
        lua_call(lua, 1, 1);
        s = lua_tostring(lua, -1);  /* get result */
        if (s == NULL)
            return luaL_error(lua, LUA_QL("tostring") " must return a string to "
                LUA_QL("print"));
        if (i>1) buf->append("\t");
        buf->append(s);
        lua_pop(lua, 1);  /* pop result */
    }
    buf->append("\n");
    return 0;
}

static int 
luaUrlEncode(lua_State *lua) {
    try {
        luaCheckStackSize(lua, 2);
        std::string value = luaReadStack<std::string>(lua, 1);
        std::string encoding = luaReadStack<std::string>(lua, 2);

        std::auto_ptr<Encoder> encoder = Encoder::createEscaping("utf-8", encoding.c_str());
        std::string encoded;
        encoder->encode(createRange(value), encoded);

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
        luaCheckStackSize(lua, 2);
        std::string value = luaReadStack<std::string>(lua, 1);
        std::string encoding = luaReadStack<std::string>(lua, 2);

        std::auto_ptr<Encoder> encoder = Encoder::createEscaping(encoding.c_str(), "utf-8");

        std::string decoded;
        encoder->encode(StringUtils::urldecode(value), decoded);
        lua_pushstring(lua, decoded.c_str());
        // Our value on stack
        return 1;
    }
    catch (const std::exception &e) {
        log()->error("caught exception in [xscript:urlencode]: %s", e.what());
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


    lua_pop(lua, 2); // pop _G and xscript

    log()->debug("%s, <<<stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
}

LuaSharedContext create_lua(Context *ctx, std::string &buffer) {
    LuaSharedContext lua_context(new LuaState(luaL_newstate()));
    lua_State * lua = lua_context->lua.get();
    luaL_openlibs(lua);

    setupXScript(lua, &buffer);

    Request *request = ctx->request();

    setupUserdata(lua, request, "request", getRequestLib());
    setupUserdata(lua, ctx->state().get(), "state", getStateLib());
    setupUserdata(lua, ctx->response(), "response", getResponseLib());

    registerCookieMethods(lua);

    return lua_context;
}

XmlDocHelper
LuaBlock::call(Context *ctx, boost::any &) throw (std::exception) {
    
    log()->entering(BOOST_CURRENT_FUNCTION);

    lua_State * lua = 0;
    
    // Buffer to store output from lua
    std::string buffer;
    
    // Try to fetch previously created lua interpret. If failed - create new one.
    boost::function<LuaSharedContext ()> creator = boost::bind(&create_lua, ctx, boost::ref(buffer));

    LuaSharedContext lua_context = ctx->param(
        std::string("xscript.lua"), 
        creator
    );
    lua = lua_context->lua.get();

    // Lock interpreter during processing.
    boost::mutex::scoped_lock lock(lua_context->mutex);

    // Top function of stack is error reporting function.
    lua_pushcfunction(lua, &luaReportError);

    if (LUA_ERRMEM == luaL_loadstring(lua, code_)) {
        throw std::bad_alloc();
    }


    int res = lua_pcall(lua, 0, LUA_MULTRET, 1);
    if (res != 0) {
        std::string msg = luaReadStack<std::string>(lua, 1);
        log()->error("%s, Lua block failed: %s", BOOST_CURRENT_FUNCTION, msg.c_str());
        throw std::runtime_error(msg);
    }
    
    XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != doc.get());

    log()->debug("Lua output: %s", buffer.c_str());
    XmlNodeHelper node(xmlNewDocNode( doc.get(), 0, BAD_CAST "lua", BAD_CAST buffer.c_str()));

    xmlDocSetRootElement(doc.get(), node.get());
    node.release();

    return doc;
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

