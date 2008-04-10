#ifndef _XSCRIPT_COMPONENT_H_
#define _XSCRIPT_COMPONENT_H_

#include <string>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/checked_delete.hpp>
#include "xscript/resource_holder.h"

namespace xscript
{

class Loader;
class Config;


template<typename Type>
class ComponentRegisterer
{
public:
	ComponentRegisterer(Type *var);
};


class ComponentBase : private boost::noncopyable
{
public:
	ComponentBase();
	virtual ~ComponentBase();

	inline boost::shared_ptr<Loader> loader() const {
		return loader_;
	}

	virtual void init(const Config *config) { 
		(void)config;
	}

private:
	boost::shared_ptr<Loader> loader_;
};


template<typename Type> 
class Component : public ComponentBase
{
public:
	static Type* instance();

    struct ResourceTraits
    {
        static Type* const DEFAULT_VALUE;
        static void destroy(Type * ptr);
    };

	typedef ResourceHolder<Type*, ResourceTraits> Holder;
	
private:
	static void attachImpl(Holder helper);
	friend class ComponentRegisterer<Type>;
	
private:
	static Holder holder_;

};


/**
 * Implementation of ResourceHolderTraits used in ComponentHolder
 */
template<typename Type>
void Component<Type>::ResourceTraits::destroy(Type *component) {
    // Acquire loader to avoid premature unload of shared library.
    boost::shared_ptr<Loader> loader = component->loader();
    boost::checked_delete(component);
};

template<typename Type>
Type* const Component<Type>::ResourceTraits::DEFAULT_VALUE = static_cast<Type*>(NULL);


template<typename Type> inline Type*
Component<Type>::instance() {
	assert(Holder::Traits::DEFAULT_VALUE != holder_.get());
	return holder_.get();
}

template<typename Type> inline void
Component<Type>::attachImpl(typename Component<Type>::Holder holder) {
	assert(Holder::Traits::DEFAULT_VALUE != holder.get());
	holder_ = holder;
}

template<typename Type>
ComponentRegisterer<Type>::ComponentRegisterer(Type *var) {
	Component<Type>::attachImpl(typename Component<Type>::Holder(var));
}

// We can't use default template instantiation. Gcc are too dumb to order
// constructors in correct order...
#define REGISTER_COMPONENT(TYPE)								\
	template<typename Type>										\
	typename Component<Type>::Holder Component<Type>::holder_;	\
																\
	template Component<TYPE>::Holder Component<TYPE>::holder_;	\
																\
	static ComponentRegisterer<TYPE> reg_(new TYPE());

#define REGISTER_COMPONENT2(TYPE, IMPL)								\
	template<typename Type>										\
	typename Component<Type>::Holder Component<Type>::holder_;	\
																\
	template Component<TYPE>::Holder Component<TYPE>::holder_;	\
																\
	static ComponentRegisterer<TYPE> reg_(new IMPL());


} // namespace xscript

#endif // _XSCRIPT_COMPONENT_H_
