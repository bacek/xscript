#include "settings.h"

#include <string.h>

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
#include "xscript/message_interface.h"
#include "xscript/operation_mode.h"
#include "xscript/profiler.h"
#include "xscript/xml.h"

#include "lua_block.h"
#include "stack.h"
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
typedef std::list<std::string> LuaStackTrace;

struct LuaState {
    std::string buffer;
    LuaStackTrace trace;
    LuaHolder state;

    LuaState(lua_State *l) : state(l) {
    }
};

typedef boost::shared_ptr<LuaState> LuaSharedContext;

class LuaThread {
public:
    LuaThread(lua_State *parent, Context::MutexPtr mutex) :
        parent_(parent), state_(lua_newthread(parent)), mutex_(mutex) {

        lua_pushvalue(parent, -1);
        thread_id_ = luaL_ref(parent, LUA_REGISTRYINDEX);
        lua_newtable(parent);
        lua_pushvalue(parent, -1);
        lua_setmetatable(parent, -2);
        lua_pushvalue(parent, LUA_GLOBALSINDEX);
        lua_setfield(parent, -2, "__index");
        lua_setfenv(parent, -2);
        lua_pop(parent, 1);
    }

    ~LuaThread() {
        boost::mutex::scoped_lock lock(*mutex_);
        luaL_unref(parent_, LUA_REGISTRYINDEX, thread_id_);
    }

    lua_State* get() const {
        return state_;
    }

private:
    LuaThread(const LuaThread &);
    LuaThread& operator = (const LuaThread &);

    lua_State* parent_;
    lua_State* state_;
    int thread_id_;
    Context::MutexPtr mutex_;
};

typedef boost::shared_ptr<LuaThread> LuaSharedThread;

LuaBlock::LuaBlock(const LuaExtension *ext, Xml *owner, xmlNodePtr node) :
        RenamedBlock(ext, owner, node), ext_(ext) {
}

LuaBlock::~LuaBlock() {
}

const std::string&
LuaBlock::getBase() const {
    return Block::getBase();
}

void
LuaBlock::postParse() {

    if (!XmlUtils::loadScriptCode(node(), code_) || code_.empty()) {
        return;
    }

    LuaHolder lua(luaL_newstate());
    int res = luaL_loadstring(lua.get(), code_.c_str());
    if (LUA_ERRSYNTAX == res) {
        std::string msg(lua_tostring(lua.get(), -1));
        throw std::runtime_error("bad lua code: " + msg);
    }
    if (LUA_ERRMEM == res) {
        throw std::bad_alloc();
    }
}
void
LuaExtension::registerLib(lua_State *lua, const char *name, bool builtin,
        const struct luaL_reg *lib_methods, const struct luaL_reg *lib_funcs) {

    std::string tableName = "xscript.";
    tableName += name;

    luaL_newmetatable(lua, tableName.c_str());
    lua_pushstring(lua, "__index");
    lua_pushvalue(lua, -2);  /* pushes the metatable */
    lua_settable(lua, -3);  /* metatable.__index = metatable */
    
    if (lib_methods) {
        luaL_openlib(lua, NULL, lib_methods, NULL);
    }
    
    if (lib_funcs) {
        luaL_openlib(lua, tableName.c_str(), lib_funcs, NULL);
    }
    
    if (builtin) {
        // Get global xscript. We will set 'name' field later
        lua_getglobal(lua, "xscript");
        pointer<void> *p = (pointer<void> *)lua_newuserdata(lua, sizeof(pointer<void>));
        p->ptr = (void*)1;
        luaL_getmetatable(lua, tableName.c_str());
        lua_setmetatable(lua, -2); // points to new userdata
        // Our userdata is on top of stack.
        // Assign it to 'name'
        lua_setfield(lua, -2, name);
        // And remove it from stack
        lua_remove(lua, -1);
    }

    lua_pop(lua, 2);
}

static void
setupLocalData(lua_State * lua, InvokeContext *invoke_ctx, Context *ctx, const LuaBlock *block) {
    lua_getglobal(lua, "xscript");

    pointer<InvokeContext> *pictx = (pointer<InvokeContext> *)lua_newuserdata(
            lua, sizeof(pointer<InvokeContext>));
    pictx->ptr = invoke_ctx;
    lua_setfield(lua, -2, "_invoke_ctx");

    pointer<Context> *pctx = (pointer<Context> *)lua_newuserdata(lua, sizeof(pointer<Context>));
    pctx->ptr = ctx;
    lua_setfield(lua, -2, "_ctx");
    
    pointer<const LuaBlock> *pblock = (pointer<const LuaBlock> *)lua_newuserdata(lua, sizeof(pointer<const LuaBlock>));
    pblock->ptr = block;
    lua_setfield(lua, -2, "_block");
}

static LuaStackTrace*
getTrace(lua_State *lua) {
    lua_getglobal(lua, "xscript");
    lua_getfield(lua, -1, "_trace");
    pointer<LuaStackTrace> *p = (pointer<LuaStackTrace>*)lua_touserdata(lua, -1);
    assert(p);
    LuaStackTrace* trace = p->ptr;
    assert(trace);
    lua_pop(lua, 2);
    return trace;
}

extern "C" void
luaHook(lua_State *lua, lua_Debug *ar) {
    LuaStackTrace* trace = getTrace(lua);
    lua_getinfo(lua, "Sln", ar);
    if (LUA_HOOKCALL == ar->event) {
        std::string name;
        if (ar->name) {
            name = std::string("function ") + ar->name;
        }
        else {
            name = "[MAIN]";
        }
        trace->push_front(name);
    }
    else if (LUA_HOOKRET == ar->event) {
        if (trace->size() > 0) {
            trace->pop_front();
        }
    }
}

static void
setupDebug(lua_State *lua, LuaStackTrace *trace) {
    lua_getglobal(lua, "xscript");
    pointer<LuaStackTrace> *pl = (pointer<LuaStackTrace> *)
        lua_newuserdata(lua, sizeof(pointer<LuaStackTrace>));
    pl->ptr = trace;
    lua_setfield(lua, -2, "_trace");
}

static LuaSharedContext
createLua(const LuaExtension *extension) {
    LuaSharedContext lua_context(new LuaState(luaL_newstate()));
    lua_State *lua = lua_context->state.get();
    luaL_openlibs(lua);
    setupXScript(lua, &(lua_context->buffer));
    extension->registerExtensions(lua);
    if (!OperationMode::instance()->isProduction()) {
        setupDebug(lua, &(lua_context->trace));
        lua_sethook(lua, &luaHook, LUA_MASKCALL | LUA_MASKRET, 0);
    }
    return lua_context;
}

static LuaSharedThread
createLuaThread(lua_State *parent, Context::MutexPtr mutex) {
    return LuaSharedThread(new LuaThread(parent, mutex));
}

void
LuaBlock::processEmptyLua(InvokeContext *invoke_ctx) const {
    XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != doc.get());
    XmlNodeHelper node(xmlNewDocNode(
        doc.get(), NULL, (const xmlChar*)"lua", (const xmlChar*)""));
    xmlDocSetRootElement(doc.get(), node.release());
    invoke_ctx->resultDoc(doc);
}

void
LuaBlock::processLuaError(lua_State *lua) const {
    std::string msg(lua_tostring(lua, -1));
    lua_pop(lua, 1);
    // Just throw exception. It will be logger by Block.

    if (OperationMode::instance()->isProduction()) {
        throw InvokeError(msg);
    }

    XmlNodeHelper trace_node(xmlNewNode(NULL, (const xmlChar*)"stacktrace"));
    LuaStackTrace* trace = getTrace(lua);
    for (LuaStackTrace::iterator it = trace->begin(), end = trace->end();
         it != end;
         ++it) {
        XmlNodeHelper node(xmlNewNode(NULL, (const xmlChar*)"level"));
        xmlNodeSetContent(node.get(), (const xmlChar*)it->c_str());
        xmlAddChild(trace_node.get(), node.release());
    }
    throw InvokeError(msg, trace_node);
}

void
LuaBlock::call(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const throw (std::exception) {
    log()->entering(BOOST_CURRENT_FUNCTION);    
    
    PROFILER(log(), "Lua block execution, " + logInfo());

    CallResultWrapper wrapper(*this, *invoke_ctx);
    
    if (code_.empty()) {
        processEmptyLua(invoke_ctx.get());
        return;
    }   
    
    Context *root_ctx = ctx->rootContext();
    Context *orig_ctx = ctx->originalStateContext();
    
    Context::MutexPtr mutex = root_ctx->param<Context::MutexPtr>(LUA_CONTEXT_MUTEX);
    boost::function<LuaSharedContext ()> lua_creator = boost::bind(&createLua, ext_);
    
    boost::mutex::scoped_lock lock(*mutex);
    
    LuaSharedContext lua_context = root_ctx->param(XSCRIPT_LUA, lua_creator);    
    lua_State *lua = lua_context->state.get();
    LuaSharedThread lua_thread;
    
    if (orig_ctx != root_ctx) {
        boost::function<LuaSharedThread ()> creator = boost::bind(&createLuaThread, lua, mutex);
        lua_thread = orig_ctx->param(XSCRIPT_THREAD, creator);
        lua = lua_thread->get();
    }
    
    lua_context->buffer.clear();
    lua_context->trace.clear();
    
    setupLocalData(lua, invoke_ctx.get(), ctx.get(), this);
    
    if (LUA_ERRMEM == luaL_loadstring(lua, code_.c_str())) {
        throw std::bad_alloc();
    }

    int res = lua_pcall(lua, 0, LUA_MULTRET, 0);    
    if (res != 0) {
        processLuaError(lua);
        return;
    }

    const char *ret_buf = lua_tostring(lua, -1);
    std::string ret;
    if (ret_buf) {
        size_t len = strlen(ret_buf);
        ret.reserve(len + 12);
        ret.assign("<lua>", 5).append(ret_buf, len).append("</lua>", 6);
    }

    std::string lua_content_str;
    lua_context->buffer.swap(lua_content_str);
    lock.unlock();

    XmlNodeHelper lua_content_node;
    if (!lua_content_str.empty()) {
        lua_content_node = XmlNodeHelper(xmlNewTextLen((const xmlChar*)lua_content_str.c_str(), lua_content_str.size()));
    }

    XmlDocHelper ret_doc;
    if (!ret.empty()) {
        ret_doc = XmlDocHelper(xmlReadMemory(ret.c_str(), ret.size(),
            "", NULL, XML_PARSE_DTDATTR | XML_PARSE_NOENT));
        if (NULL == ret_doc.get()) {
            OperationMode::instance()->processError(
                "Invalid lua output: " + XmlUtils::getXMLError());
        }
    }
    if (NULL == ret_doc.get() || NULL == xmlDocGetRootElement(ret_doc.get())) {
        if (NULL == ret_doc.get()) {
            ret_doc = XmlDocHelper(xmlNewDoc((const xmlChar*) "1.0"));
            XmlUtils::throwUnless(NULL != ret_doc.get());
        }
        XmlNodeHelper root(xmlNewNode(NULL, (const xmlChar*)"lua"));
        XmlUtils::throwUnless(NULL != root.get());
        if (lua_content_node.get()) {
            xmlAddChild(root.get(), lua_content_node.release());
        }
        xmlDocSetRootElement(ret_doc.get(), root.release());
    }
    else if (NULL != lua_content_node.get()) {
        xmlNodePtr root = xmlDocGetRootElement(ret_doc.get());
        root->children ? xmlAddPrevSibling(root->children, lua_content_node.release()) :
            xmlAddChild(root, lua_content_node.release());
    }

    invoke_ctx->resultDoc(ret_doc);
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
    MessageParam<std::vector<LuaRegisterFunc> > param(&funcs_);
    MessageParamBase* param_list[1];
    param_list[0] = &param;
    MessageParams params(1, param_list);
    MessageResultBase result;
    MessageProcessor::instance()->process("REGISTER_LUA_EXTENSION", params, result);
}

void
LuaExtension::registerExtensions(lua_State *lua) const {
    for (std::vector<LuaRegisterFunc>::const_iterator it = funcs_.begin();
         it != funcs_.end();
         ++it) {
        (*it)(lua);
    }
}

static ExtensionRegisterer reg_(ExtensionHolder(new LuaExtension()));

} // namespace xscript

extern "C" ExtensionInfo* get_extension_info() {
    static ExtensionInfo info = { "lua", xscript::XmlUtils::XSCRIPT_NAMESPACE };
    return &info;
}

