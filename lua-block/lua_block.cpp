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

#include "cookie_methods.h"
#include "local_methods.h"
#include "logger_methods.h"
#include "lua_block.h"
#include "meta_methods.h"
#include "request_methods.h"
#include "response_methods.h"
#include "stack.h"
#include "state_methods.h"
#include "xscript_methods.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

static const std::string LUA_CONTEXT_MUTEX = "LUA_CONTEXT_MUTEX";
static const std::string XSCRIPT_LUA = "xscript.lua";
static const std::string XSCRIPT_THREAD = "xscript.lua_thread";

template<>
inline void ResourceHolderTraits<lua_State*>::destroy(lua_State *state) {
    lua_close(state);
}

typedef ResourceHolder<lua_State*> LuaHolder;

struct LuaState {
    std::string buffer;
    LuaHolder state;

    LuaState(lua_State *l) : state(l) {
    }
};

typedef boost::shared_ptr<LuaState> LuaSharedContext;

class LuaThread {
public:
    LuaThread(lua_State *parent) {
        state_ = lua_newthread(parent);
        lua_pushvalue(parent, -1);
        lua_newtable(parent);
        lua_pushvalue(parent, -1);
        lua_setmetatable(parent, -2);
        lua_pushvalue(parent, LUA_GLOBALSINDEX);
        lua_setfield(parent, -2, "__index");
        lua_setfenv(parent, -2);
        lua_pop(parent, 1);
    }
    
    ~LuaThread() {}
    
    lua_State* get() const {
        return state_;
    }
    
private:
    lua_State* state_;
};

typedef boost::shared_ptr<LuaThread> LuaSharedThread;

LuaBlock::LuaBlock(const Extension *ext, Xml *owner, xmlNodePtr node) :
        Block(ext, owner, node), code_(NULL) {
}

LuaBlock::~LuaBlock() {
}

void
LuaBlock::postParse() {
    code_ = XmlUtils::cdataValue(node());
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
template <typename Type>
void
registerLibs(lua_State *lua, const char *name, Type *type,
        const struct luaL_reg *lib, const struct luaL_reg *named_lib) {
    
    log()->debug("%s, >>>stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));

    std::string tableName = "xscript.";
    tableName += name;

    luaL_newmetatable(lua, tableName.c_str());
    lua_pushstring(lua, "__index");
    lua_pushvalue(lua, -2);  /* pushes the metatable */
    lua_settable(lua, -3);  /* metatable.__index = metatable */
    
    if (NULL != lib) {
        luaL_openlib(lua, NULL, lib, NULL);
    }
    
    if (NULL != named_lib) {
        luaL_openlib(lua, tableName.c_str(), named_lib, NULL);
    }
    
    if (type) {
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
    }

    lua_pop(lua, 2);
    
    log()->debug("%s, <<<stack size is: %d", BOOST_CURRENT_FUNCTION, lua_gettop(lua));
}

static void
setupLocalData(lua_State * lua, InvokeContext *invoke_ctx, Context *ctx, const Block *block) {
    lua_getglobal(lua, "xscript");

    pointer<InvokeContext> *pictx = (pointer<InvokeContext> *)lua_newuserdata(
            lua, sizeof(pointer<InvokeContext>));
    pictx->ptr = invoke_ctx;
    lua_setfield(lua, -2, "_invoke_ctx");

    pointer<Context> *pctx = (pointer<Context> *)lua_newuserdata(lua, sizeof(pointer<Context>));
    pctx->ptr = ctx;
    lua_setfield(lua, -2, "_ctx");
    
    pointer<const Block> *pblock = (pointer<const Block> *)lua_newuserdata(lua, sizeof(pointer<const Block>));
    pblock->ptr = block;
    lua_setfield(lua, -2, "_block");
}

static LuaSharedContext
createLua() {
    LuaSharedContext lua_context(new LuaState(luaL_newstate()));
    lua_State *lua = lua_context->state.get();
    luaL_openlibs(lua);
    setupXScript(lua, &(lua_context->buffer));    
    registerLibs(lua, "request", (void*)1, getRequestLib(), getRequestLib());   
    registerLibs(lua, "state", (void*)1, getStateLib(), getStateLib());
    registerLibs(lua, "response", (void*)1, getResponseLib(), getResponseLib());
    registerLibs(lua, "localargs", (void*)1, getLocalLib(), getLocalLib());
    registerLibs(lua, "meta", (void*)1, getMetaLib(), getMetaLib());
    registerLibs(lua, "localmeta", (void*)1, getLocalMetaLib(), getLocalMetaLib());
    registerLibs(lua, "cookie", (void*)NULL, getCookieLib(), getCookieNewLib());
    registerLibs(lua, "logger", (void*)NULL, NULL, getLoggerLib());
    return lua_context;
}

static LuaSharedThread
createLuaThread(lua_State *parent) {
    return LuaSharedThread(new LuaThread(parent));
}

void
LuaBlock::call(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const throw (std::exception) {
    log()->entering(BOOST_CURRENT_FUNCTION);    
    
    PROFILER(log(), "Lua block execution, " + owner()->name());
    
    if (NULL == code_) {
        XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
        XmlUtils::throwUnless(NULL != doc.get());
        XmlNodeHelper node(xmlNewDocNode(doc.get(), NULL, (const xmlChar*) "lua", (const xmlChar*) ""));
        xmlDocSetRootElement(doc.get(), node.release());
        invoke_ctx->resultDoc(doc);
        return;
    }   
    
    Context *root_ctx = ctx->rootContext();
    Context *orig_ctx = ctx->originalContext();
    
    Context::MutexPtr mutex = root_ctx->param<Context::MutexPtr>(LUA_CONTEXT_MUTEX);
    boost::function<LuaSharedContext ()> lua_creator(&createLua);
    
    boost::mutex::scoped_lock lock(*mutex);
    
    LuaSharedContext lua_context = root_ctx->param(XSCRIPT_LUA, lua_creator);    
    lua_State *lua = lua_context->state.get();
    LuaSharedThread lua_thread;
    
    if (orig_ctx != root_ctx) {
        boost::function<LuaSharedThread ()> creator = boost::bind(&createLuaThread, lua);    
        lua_thread = orig_ctx->param(XSCRIPT_THREAD, creator);
        lua = lua_thread->get();
    }
    
    lua_context->buffer.clear();
    
    setupLocalData(lua, invoke_ctx.get(), ctx.get(), this);
    
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
    invoke_ctx->resultDoc(doc);
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
    if (ctx->isRoot()) {
        Context::MutexPtr context_mutex(new boost::mutex());
        ctx->param(LUA_CONTEXT_MUTEX, context_mutex);
    }
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

