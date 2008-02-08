#include "settings.h"

#include <vector>
#include <iostream>
#include <stdexcept>
#include <algorithm>

#include <boost/bind.hpp>
#include <boost/thread/once.hpp>
#include <boost/current_function.hpp>

#include <dlfcn.h>

#include "details/loader.h"
#include "xscript/config.h"
#include "xscript/resource_holder.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

template<>
void ResourceHolderTraits<void*>::destroy(void* handle) {
	if (NULL != handle) {
		dlclose(handle);
    }
};

typedef ResourceHolder<void*> Handle;

class LoaderImpl : public Loader
{
public:
	virtual ~LoaderImpl();

	virtual void load(const char *name);
	virtual void init(const Config *config);
	
	static boost::shared_ptr<Loader> instance();

protected:
	LoaderImpl();
	void checkLoad(const char *err);

private:
	LoaderImpl(const LoaderImpl &);
	LoaderImpl& operator = (const LoaderImpl &);

private:
	std::vector<void*> handles_;
	static boost::shared_ptr<Loader> instance_;
};

boost::shared_ptr<Loader> LoaderImpl::instance_(new LoaderImpl());

Loader::Loader()
{
}

Loader::~Loader() {
}

boost::shared_ptr<Loader>
Loader::instance() {
	return LoaderImpl::instance();
}

LoaderImpl::LoaderImpl()
{
}

LoaderImpl::~LoaderImpl() {
	std::for_each(handles_.begin(), handles_.end(), boost::bind(&dlclose, _1));
}
	
boost::shared_ptr<Loader>
LoaderImpl::instance() {
	return instance_;
}
	
void
LoaderImpl::init(const Config *config) {
	
	std::vector<std::string> v;
	std::string key("/xscript/modules/module");
	
	config->subKeys(key, v);
	for (std::vector<std::string>::iterator i = v.begin(), end = v.end(); i != end; ++i) {
		std::string module = (*i) + "/path";
		load(config->as<std::string>(module).c_str());
	}
}

void
LoaderImpl::load(const char *name) {
	Handle handle(dlopen(name, RTLD_NOW | RTLD_GLOBAL));
	
	checkLoad(dlerror());
	handles_.push_back(handle.release());

}

void
LoaderImpl::checkLoad(const char *err) {
	if (NULL != err) {
		throw std::runtime_error(err);
	}
}


} // namespace xscript
