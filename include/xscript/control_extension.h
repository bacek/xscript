#ifndef _XSCRIPT_CONTROL_EXTENSION_H_
#define _XSCRIPT_CONTROL_EXTENSION_H_

#include <map>
#include <boost/function.hpp>

#include "xscript/extension.h"

namespace xscript
{

/**
 * Internal xscript controlling point
 */
class ControlExtension : public Extension
{
public:
	ControlExtension();
	virtual ~ControlExtension();
	
	virtual const char* name() const;
	virtual const char* nsref() const;
	
	virtual void initContext(Context *ctx);
	virtual void stopContext(Context *ctx);
	virtual void destroyContext(Context *ctx);
	
	virtual std::auto_ptr<Block> createBlock(Xml *owner, xmlNodePtr node);
};


class ControlExtensionRegistry
{
public:

    typedef boost::function<std::auto_ptr<Block> (const Extension *ext, Xml*, xmlNodePtr)> constructor_t;

    static void registerConstructor(const std::string & method, constructor_t ctor);

	/**
	 * Find constructor for method. Throws and exception in case of non-existen method.
	 */
	static constructor_t findConstructor(const std::string& method);	

private:
    typedef std::map<std::string, constructor_t> constructorMap_t;
    static constructorMap_t constructors_;
};

}

#endif
