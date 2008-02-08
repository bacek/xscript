#ifndef _XSCRIPT_RESOURCE_HOLDER_H_
#define _XSCRIPT_RESOURCE_HOLDER_H_

#include <cassert>
#include <algorithm>

#include <boost/static_assert.hpp>

namespace xscript
{

/**
 * Traits for ResourceHolder.
 * Defines:
 *  - DEFAULT_VALUE constant for default initialization of resource in
 *  ResourceHolder.
 *  - destroy method for releasing acquired resource.
 */
template<typename Type>
struct ResourceHolderTraits
{
	static Type const DEFAULT_VALUE;
    static void destroy(Type ptr)
    {
        // Enforce user to specialize template.
        boost::STATIC_ASSERTION_FAILURE<false>();
    }
};

/**
 * In most cases ResourceHolder stores pointer. So defines DEFAULT_VALUE into
 * null pointer.
 */
template<typename Type>
Type const ResourceHolderTraits<Type>::DEFAULT_VALUE = static_cast<Type>(NULL);

/**
 * ResourceHolder.
 * Wraps C-style resources into RAII idiom.
 */
template<typename Type, typename ATraits = ResourceHolderTraits<Type> >
class ResourceHolder
{
public:
    typedef ATraits Traits;

	ResourceHolder();
	explicit ResourceHolder(Type tptr);
	
	ResourceHolder(const ResourceHolder<Type, Traits> &h);
	ResourceHolder<Type, Traits>& operator = (const ResourceHolder<Type, Traits> &h);
	
	~ResourceHolder() throw ();

	Type get() const;
	Type operator -> () const;
	
	Type release() const;
	void reset(Type tptr);

private:
	mutable Type tptr_;
};

template<typename Type, typename Traits> inline
ResourceHolder<Type, Traits>::ResourceHolder() : 
	tptr_(Traits::DEFAULT_VALUE)
{
}

template<typename Type, typename Traits> inline
ResourceHolder<Type, Traits>::ResourceHolder(Type tptr) :
	tptr_(tptr)
{
}

template<typename Type, typename Traits> inline 
ResourceHolder<Type, Traits>::ResourceHolder(const ResourceHolder<Type, Traits> &h) :
	tptr_(Traits::DEFAULT_VALUE)
{
	std::swap(tptr_, h.tptr_);
	assert(Traits::DEFAULT_VALUE == h.get());
}

template<typename Type, typename Traits> inline ResourceHolder<Type, Traits>&
ResourceHolder<Type, Traits>::operator = (const ResourceHolder<Type, Traits> &h) {
	if (&h != this) {
		reset(h.release());
		assert(Traits::DEFAULT_VALUE == h.get());
	}
	return *this;
}

template<typename Type, typename Traits> inline 
ResourceHolder<Type, Traits>::~ResourceHolder() throw () {
	reset(Traits::DEFAULT_VALUE);
}

template<typename Type, typename Traits> inline Type
ResourceHolder<Type, Traits>::get() const {
	return tptr_;
}

template<typename Type, typename Traits> inline Type
ResourceHolder<Type, Traits>::operator -> () const {
	assert(Traits::DEFAULT_VALUE != tptr_);
	return tptr_;
}


template<typename Type, typename Traits> inline void
ResourceHolder<Type, Traits>::reset(Type tptr) {
	if (Traits::DEFAULT_VALUE != tptr_) {
		Traits::destroy(tptr_);
	}
	tptr_ = tptr;
}

template<typename Type, typename Traits> inline Type
ResourceHolder<Type, Traits>::release() const {
	Type ptr = Traits::DEFAULT_VALUE;
	std::swap(ptr, tptr_);
	return ptr;
}

} // namespace xscript

#endif // _XSCRIPT_HELPERS_H_
