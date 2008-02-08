#ifndef _XSCRIPT_SCRIPT_CACHE_H_
#define _XSCRIPT_SCRIPT_CACHE_H_

#include <string>
#include <boost/shared_ptr.hpp>
#include <xscript/component.h>

namespace xscript
{

class Script;

class ScriptCache : 
	public virtual Component, 
	public ComponentHolder<ScriptCache>
{
public:
	ScriptCache();
	virtual ~ScriptCache();

	virtual void init(const Config *config);
	
	virtual void clear();
	virtual void erase(const std::string &name);

	virtual boost::shared_ptr<Script> fetch(const std::string &name);
	virtual void store(const std::string &name, const boost::shared_ptr<Script> &xml);
};

} // namespace xscript

#endif // _XSCRIPT_SCRIPT_CACHE_H_
