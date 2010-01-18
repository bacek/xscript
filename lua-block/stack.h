#ifndef _XSCRIPT_LUA_EXTRACT_H_
#define _XSCRIPT_LUA_EXTRACT_H_

#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <vector>

#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/static_assert.hpp>

#include <lua.hpp>

#include "xscript/string_utils.h"

namespace xscript {

class Request;
class Response;
class State;

template<typename T>
struct pointer {
    T* ptr;
};


typedef pointer<State> statePtr;
typedef pointer<Request> requestPtr;
typedef pointer<Response> responsePtr;


void luaCheckNumber(lua_State *lua, int index);
void luaCheckString(lua_State *lua, int index);
void luaCheckBoolean(lua_State *lua, int index);
void luaCheckStackSize(lua_State *lua, int index);
int luaCheckStackSize(lua_State *lua, int index_min, int index_max);

void* luaCheckUserData(lua_State *lua, const char *name, int index);

template<typename Type> inline Type
luaReadStack(lua_State *lua, int index) {
    luaCheckString(lua, index);
    return boost::lexical_cast<Type>(lua_tostring(lua, index));
}

template<typename Type> inline Type*
luaReadStack(lua_State *lua, const char *name, int index) {
    void *ud = luaCheckUserData(lua, name, index);
    return ((pointer<Type> *)ud)->ptr;
}

template<> inline bool
luaReadStack<bool>(lua_State *lua, int index) {
    luaCheckBoolean(lua, index);
    return static_cast<bool>(lua_toboolean(lua, index));
}

template<> inline boost::int32_t
luaReadStack<boost::int32_t>(lua_State *lua, int index) {
    luaCheckNumber(lua, index);
    return static_cast<boost::int32_t>(lua_tonumber(lua, index));
}

template<> inline boost::int64_t
luaReadStack<boost::int64_t>(lua_State *lua, int index) {
    luaCheckNumber(lua, index);
    return static_cast<boost::int64_t>(lua_tonumber(lua, index));
}

template<> inline boost::uint32_t
luaReadStack<boost::uint32_t>(lua_State *lua, int index) {
    luaCheckNumber(lua, index);
    return static_cast<boost::uint32_t>(lua_tonumber(lua, index));
}

template<> inline boost::uint64_t
luaReadStack<boost::uint64_t>(lua_State *lua, int index) {
    luaCheckNumber(lua, index);
    return static_cast<boost::uint64_t>(lua_tonumber(lua, index));
}

template<> inline double
luaReadStack<double>(lua_State *lua, int index) {
    luaCheckNumber(lua, index);
    return static_cast<double>(lua_tonumber(lua, index));
}

template<> inline std::string
luaReadStack<std::string>(lua_State *lua, int index) {
    luaCheckString(lua, index);
    return std::string(lua_tostring(lua, index));
}

template<typename Type>
void luaPushStack(lua_State* lua, Type t) {
    // We should specify how to push value
    boost::STATIC_ASSERTION_FAILURE<false>();
};

template<>
inline void luaPushStack(lua_State* lua, std::string str) {
    lua_pushstring(lua, str.c_str());
}

template<>
inline void luaPushStack(lua_State* lua, const std::string &str) {
    lua_pushstring(lua, str.c_str());
}


template<>
inline void luaPushStack(lua_State* lua, bool b) {
    lua_pushboolean(lua, b);
}

template<>
inline void luaPushStack(lua_State* lua, boost::int32_t n) {
    lua_pushnumber(lua, n);
}

template<>
inline void luaPushStack(lua_State* lua, boost::uint32_t n) {
    lua_pushnumber(lua, n);
}

template<>
inline void luaPushStack(lua_State* lua, boost::int64_t n) {
    lua_pushnumber(lua, n);
}

template<>
inline void luaPushStack(lua_State* lua, boost::uint64_t n) {
    lua_pushnumber(lua, n);
}

template<>
inline void luaPushStack(lua_State* lua, double n) {
    lua_pushnumber(lua, n);
}

template<>
inline void luaPushStack(lua_State* lua, std::auto_ptr<std::vector<std::string> > args) {
    int size = args->size();
    lua_createtable(lua, size, 0);
    int table = lua_gettop(lua);
    for(int i = 0; i < size; ++i) {
        lua_pushstring(lua, (*args)[i].c_str());
        lua_rawseti(lua, table, i + 1);
    }
}

template<>
inline void luaPushStack(lua_State* lua, std::auto_ptr<std::map<std::string, std::string> > args) {
    lua_newtable(lua);
    int table = lua_gettop(lua);
    for(std::map<std::string, std::string>::const_iterator it = args->begin();
        it != args->end();
        ++it) {
        lua_pushstring(lua, it->first.c_str());
        lua_pushstring(lua, it->second.c_str());
        lua_settable(lua, table);
    }
}

template<>
inline void luaPushStack(lua_State* lua, const std::vector<StringUtils::NamedValue> *args) {
    int size = args->size();
    lua_createtable(lua, size, 0);
    if (size <= 0) {
        return;
    }

    int main_table = lua_gettop(lua);
    for(int i = 0; i < size; ++i) {
        lua_newtable(lua);
        int table = lua_gettop(lua);
        const StringUtils::NamedValue& arg = args->at(i);
        lua_pushstring(lua, arg.first.c_str());
        lua_pushstring(lua, arg.second.c_str());
        lua_settable(lua, table);
        lua_rawseti(lua, main_table, i + 1);
    }
}

} // namespace xscript

#endif // _XSCRIPT_LUA_EXTRACT_H_
