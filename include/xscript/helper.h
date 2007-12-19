#ifndef _XSCRIPT_HELPERS_H_
#define _XSCRIPT_HELPERS_H_

#include <cassert>
#include <algorithm>

namespace xscript
{

template<typename Type>
struct TypeTraits
{
	static Type const DEFAULT_VALUE;
};

template<typename Type> Type const TypeTraits<Type>::DEFAULT_VALUE = static_cast<Type>(NULL);

template<typename Type, typename Traits>
class Helper
{
public:
	Helper();
	explicit Helper(Type tptr);
	
	Helper(const Helper<Type, Traits> &h);
	Helper<Type, Traits>& operator = (const Helper<Type, Traits> &h);
	
	~Helper() throw ();

	Type get() const;
	Type operator -> () const;
	
	Type release();
	void reset(Type tptr);

private:
	Type releaseInternal() const;

private:
	mutable Type tptr_;
};

template<typename Type, typename Traits> inline
Helper<Type, Traits>::Helper() : 
	tptr_(Traits::DEFAULT_VALUE)
{
}

template<typename Type, typename Traits> inline
Helper<Type, Traits>::Helper(Type tptr) :
	tptr_(tptr)
{
}

template<typename Type, typename Traits> inline 
Helper<Type, Traits>::Helper(const Helper<Type, Traits> &h) :
	tptr_(Traits::DEFAULT_VALUE)
{
	std::swap(tptr_, h.tptr_);
	assert(Traits::DEFAULT_VALUE == h.get());
}

template<typename Type, typename Traits> inline Helper<Type, Traits>&
Helper<Type, Traits>::operator = (const Helper<Type, Traits> &h) {
	if (&h != this) {
		reset(h.releaseInternal());
		assert(Traits::DEFAULT_VALUE == h.get());
	}
	return *this;
}

template<typename Type, typename Traits> inline 
Helper<Type, Traits>::~Helper() throw () {
	reset(Traits::DEFAULT_VALUE);
}

template<typename Type, typename Traits> inline Type
Helper<Type, Traits>::get() const {
	return tptr_;
}

template<typename Type, typename Traits> inline Type
Helper<Type, Traits>::operator -> () const {
	assert(Traits::DEFAULT_VALUE != tptr_);
	return tptr_;
}

template<typename Type, typename Traits> inline Type
Helper<Type, Traits>::release() {
	return releaseInternal();
}

template<typename Type, typename Traits> inline void
Helper<Type, Traits>::reset(Type tptr) {
	if (Traits::DEFAULT_VALUE != tptr_) {
		Traits::clean(tptr_);
	}
	tptr_ = tptr;
}

template<typename Type, typename Traits> inline Type
Helper<Type, Traits>::releaseInternal() const {
	Type ptr = Traits::DEFAULT_VALUE;
	std::swap(ptr, tptr_);
	return ptr;
}

} // namespace xscript

#endif // _XSCRIPT_HELPERS_H_
