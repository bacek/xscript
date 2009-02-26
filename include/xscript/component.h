#ifndef _XSCRIPT_COMPONENT_H_
#define _XSCRIPT_COMPONENT_H_

#include <map>
#include <string>
#include <typeinfo>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/checked_delete.hpp>
#include "xscript/resource_holder.h"

#include <iostream>

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
    
    typedef ResourceHolder<ComponentBase*, ResourceTraits> ResHolder;
//    typedef boost::shared_ptr<ResHolder> Holder;

protected:
    typedef std::map<std::string, boost::shared_ptr<ResHolder> > ComponentMapType;
    
    static ComponentMapType& componentMap() {
        if (components_ == NULL) {
            static std::auto_ptr<ComponentMapType> map(new ComponentMapType());
            components_ = map.get();
        }
        return *components_;
    }
    
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
//    std::cout << "Component<Type>::instance(): name: " << typeid(Type).name() << std::endl;
//    std::cout << "Map address: " << &componentMap() << std::endl;
//    std::cout << "Map size: " << componentMap().size() << std::endl;
    if (it == componentMap().end()) {
        assert(false);
    }
    return dynamic_cast<Type*>(it->second->get());
}

template<typename Type> inline void
Component<Type>::createImpl() {
    ComponentMapType::const_iterator it = componentMap().find(typeid(Type).name());
    
    std::cout << "Component<Type>::createImpl(): name: " << typeid(Type).name() << std::endl;
    
    if (it != componentMap().end()) {
        std::cout << "Component already created" << std::endl;
        return;
    }
    std::cout << "Component creating ..." << std::endl;
    
    Type* type = new Type();
    
    std::cout << "New type: " << type << std::endl;
    
    ComponentBase* base_type = dynamic_cast<ComponentBase*>(type);
    
    std::cout << "Base type: " << base_type << std::endl;
    
    boost::shared_ptr<ResHolder> holder(new ResHolder(base_type));
    
    std::cout << "Component creating 2 ...: " << holder->get() << std::endl;
    std::cout << "Map address: " << &componentMap() << std::endl;
    std::cout << "Map size: " << componentMap().size() << std::endl;
    
    componentMap()[std::string(typeid(Type).name())] = holder;
    
    std::cout << "Component creating 3 ..." << std::endl;
}

template<typename Type> inline void
Component<Type>::attachImpl(Type *component) {
    assert(ResHolder::Traits::DEFAULT_VALUE != component);
    std::cout << "Component<Type>::attachImpl(): name: " << typeid(Type).name() << std::endl;
    std::cout << "Component<Type>::attachImpl(): component name: " << typeid(*component).name() << std::endl;
    std::cout << "Component: " << component << std::endl;
    std::cout << "Map address: " << &componentMap() << std::endl;
    std::cout << "Map size: " << componentMap().size() << std::endl;
    componentMap()[std::string(typeid(Type).name())] = 
        boost::shared_ptr<ResHolder>(new ResHolder(dynamic_cast<ComponentBase*>(component)));
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
