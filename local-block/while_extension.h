#ifndef _XSCRIPT_WHILE_EXTENSION_H_
#define _XSCRIPT_WHILE_EXTENSION_H_

#include <xscript/extension.h>

namespace xscript {

class WhileExtension : public Extension {
public:
    WhileExtension();
    virtual ~WhileExtension();

    virtual const char* name() const;
    virtual const char* nsref() const;

    virtual void initContext(Context *ctx);
    virtual void stopContext(Context *ctx);
    virtual void destroyContext(Context *ctx);

    virtual std::auto_ptr<Block> createBlock(Xml *owner, xmlNodePtr node);
    virtual void init(const Config *config);

private:
    WhileExtension(const WhileExtension &);
    WhileExtension& operator = (const WhileExtension &);
};

}

#endif // _XSCRIPT_WHILE_EXTENSION_H_
