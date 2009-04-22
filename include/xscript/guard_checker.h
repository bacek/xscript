#ifndef _XSCRIPT_GUARD_CHECKER_H_
#define _XSCRIPT_GUARD_CHECKER_H_

#include <string>

#include "xscript/functors.h"

#ifndef HAVE_HASHMAP
#include <map>
#endif

namespace xscript {

class Context;

typedef bool (*GuardCheckerMethod)(const Context*, const std::string&);
typedef std::map<std::string, std::pair<GuardCheckerMethod, bool>, StringCILess> GuardCheckerMethodMap;

class GuardChecker : private boost::noncopyable {
public:
    GuardChecker();
    virtual ~GuardChecker();

    static GuardChecker* instance() {
        static GuardChecker checker;
        return &checker;
    }
    
    void registerMethod(const char *type, GuardCheckerMethod method, bool allow_empty);
    GuardCheckerMethod method(const std::string &type) const;
    bool allowed(const char *type, bool is_empty) const;

private:
    GuardCheckerMethodMap methods_;
};

class GuardCheckerRegisterer : private boost::noncopyable {
public:
    GuardCheckerRegisterer(const char *type, GuardCheckerMethod method, bool allow_empty);
};
            
} // namespace xscript

#endif // _XSCRIPT_GUARD_CHECKER_H_
