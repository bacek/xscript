#ifndef _XSCRIPT_LUA_METHOD_MAP_H_
#define _XSCRIPT_LUA_METHOD_MAP_H_

#include <string>
#include <sstream>
#include <typeinfo>
#include <stdexcept>

#include "internal/hash.h"
#include "internal/hashmap.h"

#ifndef HAVE_HASHMAP
#include <map>
#endif

#include <lua.hpp>

namespace xscript {

#ifndef HAVE_HASHMAP
typedef std::map<std::string, lua_CFunction> MethodMap;
#else
typedef details::hash_map<std::string, lua_CFunction, StringHash> MethodMap;
#endif

template<typename Type>
class MethodDispatcher : private boost::noncopyable {
public:
    MethodDispatcher();
    lua_CFunction findMethod(const std::string &name) const;

private:
    void registerMethod(const std::string &name, lua_CFunction method);

private:
    MethodMap methods_;
};

template<typename Type>
MethodDispatcher<Type>::MethodDispatcher() {
}

template<typename Type> lua_CFunction
MethodDispatcher<Type>::findMethod(const std::string &name) const {
    MethodMap::const_iterator i = methods_.find(name);
    if (methods_.end() == i) {
        std::stringstream stream;
        stream << "nonexistent " << typeid(Type).name() << " method: " << name;
        throw std::runtime_error(stream.str());
    }
    else {
        return i->second;
    }
}

template<typename Type> void
MethodDispatcher<Type>::registerMethod(const std::string &name, lua_CFunction method) {
    MethodMap::iterator i = methods_.find(name);
    if (methods_.end() == i) {
        methods_[name] = method;
    }
    else {
        std::stringstream stream;
        stream << "duplicate " << typeid(Type).name() << " method: " << name;
        throw std::runtime_error(stream.str());
    }
}

} // namespace xscript

#endif // _XSCRIPT_LUA_METHOD_MAP_H_
