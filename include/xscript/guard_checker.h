#ifndef _XSCRIPT_GUARD_CHECKER_H_
#define _XSCRIPT_GUARD_CHECKER_H_

#include <string>

#include "xscript/functors.h"

#include "internal/hash.h"
#include "internal/hashmap.h"

#ifndef HAVE_HASHMAP
#include <map>
#endif

namespace xscript {

class Context;

typedef bool (*GuardCheckerMethod)(const Context *, const std::string&);

#ifndef HAVE_HASHMAP
typedef std::map<std::string, GuardCheckerMethod, StringCILess> GuardCheckerMethodMap;
#else
typedef details::hash_map<std::string, GuardCheckerMethod, StringCIHash> GuardCheckerMethodMap;
#endif

class GuardChecker : private boost::noncopyable {
public:
    GuardChecker();
    virtual ~GuardChecker();

    static GuardChecker* instance() {
        static GuardChecker checker;
        return &checker;
    }
    
    void registerMethod(const char *name, GuardCheckerMethod method);
    GuardCheckerMethod method(const char *name);

private:
    GuardCheckerMethodMap methods_;
};

class GuardCheckerRegisterer : private boost::noncopyable {
public:
    GuardCheckerRegisterer(const char *name, GuardCheckerMethod method);
};
            
} // namespace xscript

#endif // _XSCRIPT_GUARD_CHECKER_H_
