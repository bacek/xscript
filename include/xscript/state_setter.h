#include "settings.h"

#include <map>
#include <string>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <xscript/functors.h>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class State;
class StateSetterRegistrator;

class StateSetter : public boost::noncopyable {
public:
    StateSetter();
    virtual ~StateSetter();

    static std::string set(State *state,
                           const std::string &method,
                           const std::string &name,
                           const std::string &value);

    static const std::string SET_LONG;
    static const std::string SET_LONGLONG;
    static const std::string SET_DOUBLE;
    static const std::string SET_STRING;

private:
    friend class StateSetterRegistrator;
    
    typedef boost::function<std::string (State*, const std::string&, const std::string&)> StateSetterMethod;
    typedef std::map<std::string, StateSetterMethod, StringCILess> MethodMap;
    
    static MethodMap methods_;
};

} // namespace xscript
