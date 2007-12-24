#ifndef _XSCRIPT_THREAD_POOL_H_
#define _XSCRIPT_THREAD_POOL_H_

#include <boost/utility.hpp>
#include <boost/function.hpp>

#include <xscript/component.h>

namespace xscript
{

class ThreadPool : 
	public virtual Component, 
	public ComponentHolder<ThreadPool>,
	public ComponentFactory<ThreadPool>
{
public:
	ThreadPool();
	virtual ~ThreadPool();
	
	virtual void init(const Config *config);
	virtual void invoke(boost::function<void()> f);
	virtual void stop();
};

} // namespace xscript

#endif // _XSCRIPT_THREAD_POOL_H_
