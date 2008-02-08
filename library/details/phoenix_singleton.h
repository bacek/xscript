#ifndef _XSCRIPT_DETAILS_PHOENIX_SINGLETON_H_
#define _XSCRIPT_DETAILS_PHOENIX_SINGLETON_H_

#include "settings.h"

#include <memory>
#include <cerrno>
#include <cassert>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <boost/thread/once.hpp>
#include <boost/checked_delete.hpp>

#include "xscript/util.h"
#include "xscript/resource_holder.h"
#include "details/expect.h"

namespace xscript
{

template<typename Type> 
class PhoenixSingleton
{
public:
	static Type* instance();

private:
	static void initInstance();
	static void destroyInstance();
	
private:
	static Type *instance_;
	static boost::once_flag init_;
};

template<typename Type> boost::once_flag 
PhoenixSingleton<Type>::init_ = BOOST_ONCE_INIT;

template<typename Type> Type* 
PhoenixSingleton<Type>::instance_ = ResourceHolderTraits<Type*>::DEFAULT_VALUE;

template<typename Type> inline Type*
PhoenixSingleton<Type>::instance() {
	boost::call_once(&initInstance, init_);
	assert(ResourceHolderTraits<Type*>::DEFAULT_VALUE != instance_);
	return instance_;
}

template<typename Type> inline void
PhoenixSingleton<Type>::initInstance() {
	std::auto_ptr<Type> p(new Type());
	if (XSCRIPT_LIKELY(atexit(&destroyInstance) == 0)) {
		instance_ = p.release();
		assert(ResourceHolderTraits<Type*>::DEFAULT_VALUE != instance_);
	}
	else {
		std::stringstream stream;
		StringUtils::report("failed to init singleton: ", errno, stream);
		throw std::runtime_error(stream.str());
	}
}

template<typename Type> void
PhoenixSingleton<Type>::destroyInstance() {
	delete instance_;
}

} // namespace xscript

#endif // _XSCRIPT_DETAILS_PHOENIX_SINGLETON_H_
