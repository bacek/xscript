#ifndef _XSCRIPT_LOADER_H_
#define _XSCRIPT_LOADER_H_

#include "settings.h"

#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

namespace xscript
{

class Config;

class Loader : private boost::noncopyable
{
public:
	
	virtual void load(const char *name) = 0;
	virtual void init(const Config *config) = 0;
	
	static boost::shared_ptr<Loader> instance();

protected:
	Loader();
	virtual ~Loader();

private:
	friend class boost::shared_ptr<Loader>;
};

} // namespace xscript

#endif // _XSCRIPT_LOADER_H_
