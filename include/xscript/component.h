#ifndef _XSCRIPT_COMPONENT_H_
#define _XSCRIPT_COMPONENT_H_

#include <map>
#include <string>
#include <typeinfo>

#include <cstring>

#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/checked_delete.hpp>
#include "xscript/resource_holder.h"

namespace xscript {

class Loader;
class Config;

template<typename Type>
class ComponentRegisterer {
public:
    ComponentRegisterer();
};

template<typename Type>
class ComponentImplRegisterer {
public:
    ComponentImplRegisterer(Type *var);
};

class ComponentBase : private boost::noncopyable {
public:
    ComponentBase();
    virtual ~ComponentBase();

    inline boost::shared_ptr<Loader> loader() const {
        return loader_;
    }

    virtual void init(const Config *config) {
        (void)config;
    }
    
    struct ResourceTraits {
        static ComponentBase* const DEFAULT_VALUE;
        static void destroy(ComponentBase * ptr);
    };
    
    typedef ResourceHolder<ComponentBase*, ResourceTraits> Holder;

protected:
    class StringComparator {
    public:
        bool operator() (const char* s1, const char* s2) const {
            return strcmp(s1, s2) < 0;
        }
    };
    
    typedef std::map<const char*, boost::shared_ptr<Holder>, StringComparator> ComponentMapType;
    
    static ComponentMapType& componentMap();
    
private:
    static ComponentMapType* components_;
    boost::shared_ptr<Loader> loader_; 
};


template<typename Type>
class Component : public ComponentBase {
public:
    static Type* instance();
private:
    static void createImpl();
    static void attachImpl(Type *component);
    friend class ComponentRegisterer<Type>;
    friend class ComponentImplRegisterer<Type>;
};

template<typename Type> inline Type*
Component<Type>::instance() {
    ComponentMapType::const_iterator it = componentMap().find(typeid(Type).name());
    if (it == componentMap().end()) {
        assert(false);
    }
    return dynamic_cast<Type*>(it->second->get());
}

template<typename Type> inline void
Component<Type>::createImpl() {
    ComponentMapType::const_iterator it = componentMap().find(typeid(Type).name());
    if (it != componentMap().end()) {
        return;
    }
    boost::shared_ptr<Holder> holder(
            new Holder(dynamic_cast<ComponentBase*>(new Type())));
    
    componentMap()[typeid(Type).name()] = holder;
}

template<typename Type> inline void
Component<Type>::attachImpl(Type *component) {
    assert(Holder::Traits::DEFAULT_VALUE != component);
    componentMap()[typeid(Type).name()] = 
        boost::shared_ptr<Holder>(new Holder(dynamic_cast<ComponentBase*>(component)));
}

template<typename Type>
ComponentRegisterer<Type>::ComponentRegisterer() {
    Component<Type>::createImpl();
}

template<typename Type>
ComponentImplRegisterer<Type>::ComponentImplRegisterer(Type *var) {
    Component<Type>::attachImpl(var);
}

} // namespace xscript

#endif // _XSCRIPT_COMPONENT_H_
