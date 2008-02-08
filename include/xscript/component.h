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


class Component : private boost::noncopyable
{
public:
	Component();
	virtual ~Component();

	virtual void init(const Config *config) = 0;

	inline boost::shared_ptr<Loader> loader() const {
		return loader_;
	}

private:
	boost::shared_ptr<Loader> loader_;
};

/**
 * Default creation policy for ComponentHolder.
 */
template<typename Type>
struct DefaultCreationPolicy
{
	static inline Type* createImpl() {
		return new Type();
	}
};


template<typename Type, typename CP = DefaultCreationPolicy<Type> >
class ComponentRegisterer
{
public:
	ComponentRegisterer(Type *var);
};


/**
 */
template<typename Type, typename CreationPolicy = DefaultCreationPolicy<Type> > 
class ComponentHolder
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
	friend class ComponentRegisterer<Type, CreationPolicy>;
	
private:
	static Holder holder_;
};


/**
 * Implementation of ResourceHolderTraits used in ComponentHolder
 */
template<typename Type, typename CP>
void ComponentHolder<Type, CP>::ResourceTraits::destroy(Type *component) {
    // Acquire loader to avoid premature unload of shared library.
    boost::shared_ptr<Loader> loader = component->loader();
    boost::checked_delete(component);
};

template<typename Type, typename CP>
Type* const ComponentHolder<Type, CP>::ResourceTraits::DEFAULT_VALUE = static_cast<Type*>(NULL);


template<typename Type, typename CP> 
typename ComponentHolder<Type, CP>::Holder 
ComponentHolder<Type, CP>::holder_(CP::createImpl());

template<typename Type, typename CP> inline Type*
ComponentHolder<Type, CP>::instance() {
	assert(Holder::Traits::DEFAULT_VALUE != holder_.get());
	return holder_.get();
}

template<typename Type, typename CP> inline void
ComponentHolder<Type, CP>::attachImpl(typename ComponentHolder<Type, CP>::Holder holder) {
	assert(Holder::Traits::DEFAULT_VALUE != holder.get());
	holder_ = holder;
}

template<typename Type, typename CP>
ComponentRegisterer<Type, CP>::ComponentRegisterer(Type *var) {
	ComponentHolder<Type, CP>::attachImpl(typename ComponentHolder<Type, CP>::Holder(var));
}

} // namespace xscript

#endif // _XSCRIPT_COMPONENT_H_
