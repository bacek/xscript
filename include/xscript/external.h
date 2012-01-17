#ifndef _XSCRIPT_EXTERNAL_H_
#define _XSCRIPT_EXTERNAL_H_

#include <map>
#include <memory>
#include <string>

#include <boost/shared_ptr.hpp>

#include "xscript/component.h"

namespace xscript {

class Context;
class InvokeContext;
class TypedValue;

typedef unsigned int ExternalFunctionArgcType;

struct ExternalFunctionContext {

    typedef ExternalFunctionArgcType ArgcType;

    ExternalFunctionContext();

    const char *scope;
    const char *module_name;
    const char *function_name;

    Context *ctx;
    InvokeContext *invoke_ctx;

    ArgcType argc;
    TypedValue *argv;
};

// return true to check result
typedef bool (*ExternalFunction)(const ExternalFunctionContext &fctx, TypedValue &result);

struct ExternalFunctionInfo {

    typedef ExternalFunctionArgcType ArgcType;
    static const ArgcType ARGC_UNDEFINED = (ArgcType)-1;

    ExternalFunctionInfo() : fp(NULL), argc_min(0), argc_max(0) {}
    ExternalFunctionInfo(ExternalFunction a_fp, ArgcType a_min = 0, ArgcType a_max = ARGC_UNDEFINED) :
        fp(a_fp), argc_min(a_min), argc_max(a_max) {}

    enum ARITY_CHECK_RESULT {
        BAD_ARITY_LOW_BOUND = -1,
        ARITY_OK = 0,
        BAD_ARITY_HIGH_BOUND = 1,
    };

    ARITY_CHECK_RESULT checkArity(ArgcType argc) const;

    ExternalFunction fp;

    ArgcType argc_min;
    ArgcType argc_max;
};


class ExternalModule {
public:
    ExternalModule();
    virtual ~ExternalModule();

    typedef std::map<std::string, ExternalFunctionInfo> FunctionsType;
    typedef ExternalFunctionArgcType ArgcType;

    void merge(const ExternalModule &m);

    const FunctionsType& functions() const;

    void registerFunction(const std::string &name, const ExternalFunctionInfo &fi);
    void registerFunction(const std::string &name,
        ExternalFunction fp, ArgcType a_min = 0, ArgcType a_max = ExternalFunctionInfo::ARGC_UNDEFINED);

private:
    ExternalModule(const ExternalModule &);
    ExternalModule& operator = (const ExternalModule &);

    class ExternalModuleImpl;
    std::auto_ptr<ExternalModuleImpl> impl_;
};
typedef boost::shared_ptr<ExternalModule> ExternalModulePtr;


class ExternalModules : public Component<ExternalModules> {
public:
    ExternalModules();
    virtual ~ExternalModules();

    typedef std::map<std::string, ExternalModulePtr> ModulesType;

    const ModulesType& modules() const;

    void registerModule(const std::string &name, ExternalModulePtr module);
    void unregisterAll();

private:
    class ExternalModulesImpl;
    std::auto_ptr<ExternalModulesImpl> impl_;
};

} // namespace xscript

#endif // _XSCRIPT_EXTERNAL_H_
