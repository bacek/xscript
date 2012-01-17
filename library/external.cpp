#include "settings.h"

#include <cassert>

#include <string.h>

#include "xscript/external.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

struct ExternalModule::ExternalModuleImpl {
    typedef ExternalModule::FunctionsType FunctionsType;

    FunctionsType functions;
};

struct ExternalModules::ExternalModulesImpl {
    typedef ExternalModules::ModulesType ModulesType;

    ModulesType modules;
};


ExternalFunctionContext::ExternalFunctionContext() {
    memset(this, 0, sizeof(*this));
}


ExternalFunctionInfo::ARITY_CHECK_RESULT
ExternalFunctionInfo::checkArity(ArgcType argc) const {
    if (argc < argc_min) {
        return BAD_ARITY_LOW_BOUND;
    }
    if (argc_max != ARGC_UNDEFINED && argc > argc_max) {
        return BAD_ARITY_HIGH_BOUND;
    }
    return ARITY_OK;
}


ExternalModule::ExternalModule() : impl_(new ExternalModuleImpl()) {
}

ExternalModule::~ExternalModule() {
}

void
ExternalModule::merge(const ExternalModule &m) {
    const FunctionsType &funcs = m.functions();
    for (FunctionsType::const_iterator it = funcs.begin(), end = funcs.end(); it != end; ++it) {
        registerFunction(it->first, it->second);
    }
}

const ExternalModule::FunctionsType&
ExternalModule::functions() const {
    return impl_->functions;
}

void
ExternalModule::registerFunction(const std::string &name, const ExternalFunctionInfo &fi) {
    assert(!name.empty());
    assert(fi.fp);
    assert(fi.argc_max == ExternalFunctionInfo::ARGC_UNDEFINED || fi.argc_min <= fi.argc_max);
    bool ok = impl_->functions.insert(std::make_pair(name, fi)).second;    
    assert(ok);
}

void
ExternalModule::registerFunction(const std::string &name, ExternalFunction fp, ArgcType min, ArgcType max) {
    registerFunction(name, ExternalFunctionInfo(fp, min, max));
}


ExternalModules::ExternalModules() : impl_(new ExternalModulesImpl()) {
}

ExternalModules::~ExternalModules() {
}

const ExternalModules::ModulesType&
ExternalModules::modules() const {
    return impl_->modules;
}

void
ExternalModules::registerModule(const std::string &name, ExternalModulePtr module) {
    assert(!name.empty());
    assert(module.get());

    ModulesType::iterator it = impl_->modules.find(name);
    if (it == impl_->modules.end()) {
        bool ok = impl_->modules.insert(std::make_pair(name, module)).second;    
        assert(ok);
    }
    else {
        it->second->merge(*module);
    }
}

void
ExternalModules::unregisterAll() {
    impl_->modules.clear();
}


static ComponentRegisterer<ExternalModules> reg_;

} // namespace xscript
