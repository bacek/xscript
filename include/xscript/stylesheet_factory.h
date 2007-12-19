#ifndef _XSCRIPT_STYLESHEET_FACTORY_H_
#define _XSCRIPT_STYLESHEET_FACTORY_H_

#include <string>
#include <boost/shared_ptr.hpp>
#include <xscript/component.h>

namespace xscript
{

class Stylesheet;

class StylesheetFactory : 
	public virtual Component, 
	public ComponentHolder<StylesheetFactory>,
	public ComponentFactory<StylesheetFactory>
{
public:
	StylesheetFactory();
	virtual ~StylesheetFactory();
	
	virtual void init(const Config *config);
	virtual boost::shared_ptr<Stylesheet> create(const std::string &name);
};

} // namespace xscript

#endif // _XSCRIPT_STYLESHEET_FACTORY_H_
