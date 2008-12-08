#include "settings.h"

#include <vector>
#include <iostream>
#include <stdexcept>
#include <algorithm>

#include <boost/bind.hpp>
#include <boost/thread/once.hpp>
#include <boost/current_function.hpp>

#include "ltdl.h"
#include "internal/loader.h"
#include "xscript/config.h"
#include "xscript/resource_holder.h"
#include "xscript/logger_factory.h"
#include "internal/extension_list.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

template<>
void ResourceHolderTraits<lt_dlhandle>::destroy(lt_dlhandle handle) {
    if (NULL != handle) {
        lt_dlclose(handle);
    }
};

typedef ResourceHolder<lt_dlhandle> Handle;

class LoaderImpl : public Loader {
public:
    virtual ~LoaderImpl();

    virtual ExtensionInfo * load(const char *name);
    virtual void init(const Config *config);

    static boost::shared_ptr<Loader> instance();

protected:
    LoaderImpl();
    void checkLoad(bool success) const;

private:
    std::vector<lt_dlhandle> handles_;
    static boost::shared_ptr<Loader> instance_;
};

boost::shared_ptr<Loader> LoaderImpl::instance_(new LoaderImpl());

Loader::Loader() {
    if (0 != lt_dlinit()) {
        throw std::runtime_error("failed to init libltdl");
    }
}

Loader::~Loader() {
    lt_dlexit();
}

boost::shared_ptr<Loader>
Loader::instance() {
    return LoaderImpl::instance();
}

LoaderImpl::LoaderImpl() {
}

LoaderImpl::~LoaderImpl() {
    std::for_each(handles_.begin(), handles_.end(), boost::bind(&lt_dlclose, _1));
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
        ExtensionInfo * info = load(config->as<std::string>(module).c_str());

        // Set appropriate logger if we successfully fetch ExtensionInfo from module
        if (info != 0) {
            std::string loggerName = config->as<std::string>(*i + "/logger", "default");
//			std::cerr << "Logger " << loggerName << "\n";

            Extension * ext = ExtensionList::instance()->extension(info->name, info->nsref, true);
            if (ext) {
                ext->setLogger(LoggerFactory::instance()->getLogger(loggerName));
            }
        }
    }
}

ExtensionInfo *
LoaderImpl::load(const char *name) {

    typedef ExtensionInfo* (*ExtensionFunc)();
    
    Handle handle(lt_dlopen(name));
    checkLoad(NULL != handle.get());
    handles_.push_back(handle.release());
    
    ExtensionFunc func = (ExtensionFunc) lt_dlsym(handles_.back(), "get_extension_info");
    return (NULL != func) ? func() : NULL;
}

void
LoaderImpl::checkLoad(bool success) const {
    if (!success) {
        throw std::runtime_error(lt_dlerror());
    }
}


} // namespace xscript
