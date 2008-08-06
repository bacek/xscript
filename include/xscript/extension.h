#ifndef _XSCRIPT_EXTENSION_H_
#define _XSCRIPT_EXTENSION_H_

#include <memory>
#include <boost/utility.hpp>

#include "xscript/resource_holder.h"
#include "xscript/component.h"
#include <libxml/tree.h>

namespace xscript {

class Xml;
class Block;
class Config;
class Context;
class Logger;

class Extension : public virtual Component<Extension> {
public:
    Extension();
    virtual ~Extension();

    virtual const char* name() const = 0;
    virtual const char* nsref() const = 0;

    virtual void initContext(Context *ctx) = 0;
    virtual void stopContext(Context *ctx) = 0;
    virtual void destroyContext(Context *ctx) = 0;

    virtual std::auto_ptr<Block> createBlock(Xml *owner, xmlNodePtr node) = 0;

    void setLogger(Logger* logger) {
        logger_ = logger;
    }

    Logger *getLogger() const {
        return logger_;
    }

private:
    Logger * logger_;
};

typedef ResourceHolder<Extension*, Component<Extension>::ResourceTraits > ExtensionHolder;

class ExtensionRegisterer : private boost::noncopyable {
public:
    ExtensionRegisterer(ExtensionHolder holder) throw ();
};


} // namespace xscript


/**
 * Extension info. Only name for now.
 */
struct ExtensionInfo {
    const char *name;
    const char *nsref;
};

/**
 * Get extension info.
 */
extern "C" ExtensionInfo * get_extension_info();

#endif // _XSCRIPT_EXTENSION_H_
