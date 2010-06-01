#ifndef _XSCRIPT_LOCAL_EXTENSION_H_
#define _XSCRIPT_LOCAL_EXTENSION_H_

#include <xscript/extension.h>

namespace xscript {

class LocalExtension : public Extension {
public:
    LocalExtension();
    virtual ~LocalExtension();

    virtual const char* name() const;
    virtual const char* nsref() const;

    virtual void initContext(Context *ctx);
    virtual void stopContext(Context *ctx);
    virtual void destroyContext(Context *ctx);

    virtual std::auto_ptr<Block> createBlock(Xml *owner, xmlNodePtr node);
    virtual void init(const Config *config);

    virtual bool allowEmptyNamespace() const;
private:
    LocalExtension(const LocalExtension &);
    LocalExtension& operator = (const LocalExtension &);
};

}

#endif // _XSCRIPT_LOCAL_EXTENSION_H_
