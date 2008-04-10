#ifndef _XSCRIPT_SCRIPT_FACTORY_H_
#define _XSCRIPT_SCRIPT_FACTORY_H_

#include <string>
#include <boost/shared_ptr.hpp>
#include <xscript/component.h>

namespace xscript
{

class Script;

class ScriptFactory : public Component<ScriptFactory>
{
public:
	ScriptFactory();
	virtual ~ScriptFactory();

	virtual boost::shared_ptr<Script> create(const std::string &name);
};

} // namespace xscript

#endif // _XSCRIPT_SCRIPT_FACTORY_H_
