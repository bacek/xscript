#ifndef _XSCRIPT_HTTP_EXTENSION_H_
#define _XSCRIPT_HTTP_EXTENSION_H_

#include "xscript/extension.h"

namespace xscript {

class HttpExtension : public Extension {
public:
    HttpExtension();
    virtual ~HttpExtension();

    virtual const char* name() const;
    virtual const char* nsref() const;

    virtual void initContext(Context *ctx);
    virtual void stopContext(Context *ctx);
    virtual void destroyContext(Context *ctx);

    virtual std::auto_ptr<Block> createBlock(Xml *owner, xmlNodePtr node);
    virtual void init(const Config *config);

private:
    HttpExtension(const HttpExtension &);
    HttpExtension& operator = (const HttpExtension &);
};

} // namespace xscript

#endif // _XSCRIPT_HTTP_EXTENSION_H_
