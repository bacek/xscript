#ifndef _XSCRIPT_SCRIPT_CACHE_H_
#define _XSCRIPT_SCRIPT_CACHE_H_

#include <string>
#include <boost/shared_ptr.hpp>
#include <xscript/component.h>

namespace xscript {

class Script;

class ScriptCache : public Component<ScriptCache> {
public:

    virtual void clear() = 0;
    virtual void erase(const std::string &name) = 0;

    virtual boost::shared_ptr<Script> fetch(const std::string &name) = 0;
    virtual void store(const std::string &name, const boost::shared_ptr<Script> &xml) = 0;
};

} // namespace xscript

#endif // _XSCRIPT_SCRIPT_CACHE_H_
