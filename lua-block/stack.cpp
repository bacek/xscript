#include "settings.h"

#include "stack.h"
#include "exception.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

void
luaCheckNumber(lua_State *lua, int index) {
    if (!lua_isnumber(lua, index)) {
        throw BadType("number", index);
    }
}

void
luaCheckString(lua_State *lua, int index) {
    if (!lua_isstring(lua, index)) {
        throw BadType("string", index);
    }
}

void
luaCheckBoolean(lua_State *lua, int index) {
    if (!lua_isboolean(lua, index)) {
        throw BadType("boolean", index);
    }
}

int
luaCheckStackSize(lua_State *lua, int count_min, int count_max) {
    int count = lua_gettop(lua);
    if (count < count_min || count > count_max) {
        throw BadArgCount(count);
    }
    return count;
}

void
luaCheckStackSize(lua_State *lua, int count) {
    luaCheckStackSize(lua, count, count);
}

void*
luaCheckUserData(lua_State *lua, const char *name, int index) {
    void *ptr = luaL_checkudata(lua, index, name);
    if (NULL == ptr) {
        throw BadType(name, index);
    }
    luaL_argcheck(lua, ptr != NULL, index, "`ud' expected");
    return ptr;

}

} // namespace xscript
