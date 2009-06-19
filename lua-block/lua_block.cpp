#include "settings.h"

#include <new>
#include <memory>
#include <stdexcept>
#include <boost/current_function.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>

#include "xscript/xml_util.h"
#include "xscript/util.h"
#include "xscript/resource_holder.h"
#include "xscript/logger.h"
#include "xscript/context.h"
#include "xscript/profiler.h"
#include "xscript/xml.h"

#include "stack.h"
#include "lua_block.h"
#include "xscript_methods.h"
#include "state_methods.h"
#include "request_methods.h"
#include "response_methods.h"
#include "cookie_methods.h"
#include "logger_methods.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

template<>
inline void ResourceHolderTraits<lua_State*>::destroy(lua_State *state) {
    lua_close(state);
}

typedef ResourceHolder<lua_State*> LuaHolder;

// Struct to store lua_State and mutex in Context
struct LuaState {
    // Buffer to store output from lua
    std::string buffer;

    LuaHolder       lua;
    boost::mutex    mutex;

    LuaState(lua_State * l) : lua(l) {
    }
};

typedef boost::shared_ptr<LuaState> LuaSharedContext;

const std::string LuaBlock::XSCRIPT_LUA = "xscript.lua";

LuaBlock::LuaBlock(const Extension *ext, Xml *owner, xmlNodePtr node) :
        Block(ext, owner, node), code_(NULL) {
}

LuaBlock::~LuaBlock() {
}

void
LuaBlock::postParse() {

    xmlNodePtr ptr = NULL;
    for (ptr = node()->children; NULL != ptr; ptr = ptr->next) {
        if (XML_CDATA_SECTION_NODE == ptr->type) {
            code_ = (const char*) ptr->content;
            break;
        }
    }
    if (NULL == code_) {
        code_ = XmlUtils::value(node());
        if (NULL == code_) {
            return;
        }
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
}


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

    // Get global xscript. We will set 'name' field later
    lua_getglobal(lua, "xscript");

    pointer<Type> *p = (pointer<Type> *)lua_newuserdata(lua, sizeof(pointer<Type>));
    p->ptr = type;

    luaL_getmetatable(lua, tableName.c_str());
    lua_setmetatable(lua, -2); // points to new userdata

    // Our userdata is on top of stack.
    // Assign it to 'name'
    lua_setfield(lua, -2, name);

    // And remove it from stack
    lua_remove(lua, -1);
    lua_pop(lua, 2); // pop xscript, __metatable

    log()->debug("%s, <<<stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
}

LuaSharedContext create_lua(Context *ctx) {
    LuaSharedContext lua_context(new LuaState(luaL_newstate()));
    lua_State * lua = lua_context->lua.get();
    luaL_openlibs(lua);

    setupXScript(lua, &(lua_context->buffer), ctx);

    Request *request = ctx->request();

    setupUserdata(lua, request, "request", getRequestLib());
    setupUserdata(lua, ctx->state(), "state", getStateLib());
    setupUserdata(lua, ctx->response(), "response", getResponseLib());

    registerCookieMethods(lua);
    registerLoggerMethods(lua);

    return lua_context;
}

XmlDocHelper
LuaBlock::call(boost::shared_ptr<Context> ctx, boost::any &) throw (std::exception) {

    log()->entering(BOOST_CURRENT_FUNCTION);    
    
    PROFILER(log(), "Lua block execution, " + owner()->name());

    if (NULL == code_) {
        XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
        XmlUtils::throwUnless(NULL != doc.get());
        XmlNodeHelper node(xmlNewDocNode(doc.get(), NULL, (const xmlChar*) "lua", (const xmlChar*) ""));
        xmlDocSetRootElement(doc.get(), node.release());
        return doc;
    }
    
    // Try to fetch previously created lua interpret. If failed - create new one.
    boost::function<LuaSharedContext ()> creator = boost::bind(&create_lua, ctx.get());

    LuaSharedContext lua_context = ctx->param(XSCRIPT_LUA, creator);
    lua_State * lua = lua_context->lua.get();

    // Lock interpreter during processing.
    boost::mutex::scoped_lock lock(lua_context->mutex);
    lua_context->buffer.clear();

    if (LUA_ERRMEM == luaL_loadstring(lua, code_)) {
        throw std::bad_alloc();
    }

    int res = lua_pcall(lua, 0, LUA_MULTRET, 0);
    if (res != 0) {
        std::string msg(lua_tostring(lua, -1));
        lua_pop(lua, 1);
        // Just throw exception. It will be logger by Block.
        throw InvokeError(msg);
    }

    XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != doc.get());

    XmlNodeHelper node;
    if (lua_context->buffer.empty()) {
        node = XmlNodeHelper(xmlNewDocNode(doc.get(), NULL, (const xmlChar*) "lua", 
          (const xmlChar*) ""));
    }
    else {
        log()->debug("Lua output: %s", lua_context->buffer.c_str());
        node = XmlNodeHelper(xmlNewDocNode(doc.get(), NULL, (const xmlChar*) "lua",
            (const xmlChar*) XmlUtils::escape(lua_context->buffer).c_str()));
    }
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

