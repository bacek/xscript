#ifndef _XSCRIPT_STYLESHEET_CACHE_H_
#define _XSCRIPT_STYLESHEET_CACHE_H_

#include <string>
#include <boost/shared_ptr.hpp>
#include <xscript/component.h>

namespace xscript
{

class Stylesheet;

class StylesheetCache : 
	public virtual Component, 
	public ComponentHolder<StylesheetCache>
{
public:
	StylesheetCache();
	virtual ~StylesheetCache();
	
	virtual void init(const Config *config);
	
	virtual void clear();
	virtual void erase(const std::string &name);

	virtual boost::shared_ptr<Stylesheet> fetch(const std::string &name);
	virtual void store(const std::string &name, const boost::shared_ptr<Stylesheet> &stylesheet);
};

} // namespace xscript

#endif // _XSCRIPT_STYLESHEET_CACHE_H_
