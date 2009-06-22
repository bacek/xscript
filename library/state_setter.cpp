#include "settings.h"

#include <boost/lexical_cast.hpp>

#include <xscript/state.h>
#include <xscript/state_setter.h>
#include <xscript/util.h>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

static std::string
setDouble(xscript::State *state, const std::string &name, const std::string &value) {
    state->checkName(name);
    double val = 0.0;
    try {
        val = boost::lexical_cast<double>(value);
    }
    catch (const boost::bad_lexical_cast &e) {
        val = 0.0;
    }
    state->setDouble(name, val);
    return boost::lexical_cast<std::string>(val);
}

static std::string
setLong(xscript::State *state, const std::string &name, const std::string &value) {
    state->checkName(name);
    boost::int32_t val = 0;
    try {
        val = boost::lexical_cast<boost::int32_t>(value);
    }
    catch (const boost::bad_lexical_cast &e) {
        val = 0;
    }
    state->setLong(name, val);
    return boost::lexical_cast<std::string>(val);
}

static std::string
setLongLong(xscript::State *state, const std::string &name, const std::string &value) {
    state->checkName(name);
    boost::int64_t val = 0;
    try {
        val = boost::lexical_cast<boost::int64_t>(value);
    }
    catch (const boost::bad_lexical_cast &e) {
        val = 0;
    }
    state->setLongLong(name, val);
    return boost::lexical_cast<std::string>(val);
}

static std::string
setString(xscript::State *state, const std::string &name, const std::string &value) {
    state->checkName(name);
    state->setString(name, value);
    return value;
}

namespace xscript {

StateSetter::MethodMap StateSetter::methods_;

const std::string StateSetter::SET_DOUBLE = "set_state_double";
const std::string StateSetter::SET_LONG = "set_state_long";
const std::string StateSetter::SET_LONGLONG = "set_state_longlong";
const std::string StateSetter::SET_STRING = "set_state_string";


class StateSetterRegistrator {
public:
    StateSetterRegistrator();
};

StateSetter::StateSetter() {
}

StateSetter::~StateSetter() {
}

std::string
StateSetter::set(State *state,
                 const std::string &method,
                 const std::string &name,
                 const std::string &value) {
    MethodMap::iterator it = methods_.find(method);
    if (methods_.end() == it) {
        throw std::runtime_error("Unknown state setter method: " + name);
    }

    return it->second(state, name, value);
}

StateSetterRegistrator::StateSetterRegistrator() {
    StateSetter::methods_[StateSetter::SET_DOUBLE] = &setDouble;
    StateSetter::methods_[StateSetter::SET_LONG] = &setLong;
    StateSetter::methods_[StateSetter::SET_LONGLONG] = &setLongLong;
    StateSetter::methods_[StateSetter::SET_STRING] = &setString;
}

static StateSetterRegistrator reg_;

} // namespace xscript
