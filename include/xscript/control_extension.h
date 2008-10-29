#ifndef _XSCRIPT_CONTROL_EXTENSION_H_
#define _XSCRIPT_CONTROL_EXTENSION_H_

#include <map>
#include <boost/function.hpp>

#include <xscript/extension.h>

namespace xscript {

/**
 * Internal xscript controlling point
 */
class ControlExtension : public Extension {
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


class ControlExtensionRegistry {
public:

    typedef boost::function<std::auto_ptr<Block> (const Extension *ext, Xml*, xmlNodePtr)> Constructor;

    static void registerConstructor(const std::string & method, Constructor ctor);

    /**
     * Find constructor for method. Throws and exception in case of non-existen method.
     */
    static Constructor findConstructor(const std::string& method);
    static void clearConstructors();

private:
    typedef std::map<std::string, Constructor> ConstructorMap;
    static ConstructorMap constructors_;
};

} // namespace xscript

#endif // _XSCRIPT_CONTROL_EXTENSION_H_
