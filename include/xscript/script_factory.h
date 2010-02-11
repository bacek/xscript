#ifndef _XSCRIPT_SCRIPT_FACTORY_H_
#define _XSCRIPT_SCRIPT_FACTORY_H_

#include <string>
#include <boost/shared_ptr.hpp>
#include <xscript/component.h>

namespace xscript {

class Script;

class ScriptFactory : public Component<ScriptFactory> {
public:
    ScriptFactory();
    virtual ~ScriptFactory();

    static boost::shared_ptr<Script> createScript(const std::string &name);
    static boost::shared_ptr<Script> createScript(const std::string &name, const std::string &xml);
    static boost::shared_ptr<Script> createScript(xmlNodePtr node);
    
protected:
    virtual boost::shared_ptr<Script> create(const std::string &name);
    static boost::shared_ptr<Script> createWithParse(const std::string &name);
};

} // namespace xscript

#endif // _XSCRIPT_SCRIPT_FACTORY_H_
