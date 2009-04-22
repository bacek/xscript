#ifndef _XSCRIPT_GUARD_CHECKER_H_
#define _XSCRIPT_GUARD_CHECKER_H_

#include <string>

#include "xscript/functors.h"

#ifndef HAVE_HASHMAP
#include <map>
#endif

namespace xscript {

class Context;

typedef bool (*GuardCheckerMethod)(const Context *, const std::string&);
typedef std::map<std::string, GuardCheckerMethod, StringCILess> GuardCheckerMethodMap;

class GuardChecker : private boost::noncopyable {
public:
    GuardChecker();
    virtual ~GuardChecker();

    static GuardChecker* instance() {
        static GuardChecker checker;
        return &checker;
    }
    
    void registerMethod(const char *name, GuardCheckerMethod method);
    GuardCheckerMethod method(const std::string &name) const;

private:
    GuardCheckerMethodMap methods_;
};

class GuardCheckerRegisterer : private boost::noncopyable {
public:
    GuardCheckerRegisterer(const char *name, GuardCheckerMethod method);
};
            
} // namespace xscript

#endif // _XSCRIPT_GUARD_CHECKER_H_
