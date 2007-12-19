#ifndef _XSCRIPT_COMPONENT_H_
#define _XSCRIPT_COMPONENT_H_

#include <string>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/checked_delete.hpp>
#include <xscript/helper.h>

namespace xscript
{

class Loader;
class Config;
class Component;

template<typename Type>
class ComponentFactory
{
public:
	static inline Type* createImpl() {
		return new Type();
	}
};

template<typename Type>
class ComponentTraits : public TypeTraits<Type*>
{
public:
	static inline void clean(Type *type) {
		boost::shared_ptr<Loader> loader = type->loader();
		boost::checked_delete(type);
	}
};

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

template<typename Type>
class ComponentRegisterer
{
public:
	ComponentRegisterer(Type *var);
};

template<typename Type> 
class ComponentHolder
{
public:
	static Type* instance();
	typedef Helper<Type*, ComponentTraits<Type> > Helper;
	
private:
	static void attachImpl(Helper helper);
	friend class ComponentRegisterer<Type>;
	
private:
	static Helper helper_;
};

template<typename Type> typename ComponentHolder<Type>::Helper 
ComponentHolder<Type>::helper_(Type::createImpl());

template<typename Type>
ComponentRegisterer<Type>::ComponentRegisterer(Type *var) {
	ComponentHolder<Type>::attachImpl(typename ComponentHolder<Type>::Helper(var));
}

template<typename Type> inline Type*
ComponentHolder<Type>::instance() {
	assert(ComponentTraits<Type>::DEFAULT_VALUE != helper_.get());
	return helper_.get();
}

template<typename Type> inline void
ComponentHolder<Type>::attachImpl(typename ComponentHolder<Type>::Helper helper) {
	assert(ComponentTraits<Type>::DEFAULT_VALUE != helper.get());
	helper_ = helper;
}

} // namespace xscript

#endif // _XSCRIPT_COMPONENT_H_
