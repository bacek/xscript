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

    static bool checkedHeaders() { return checked_headers_; }
    static bool checkedQueryParams() { return checked_query_params_; }
    static bool loadEntities() { return load_entities_; }

private:
    HttpExtension(const HttpExtension &);
    HttpExtension& operator = (const HttpExtension &);

    static bool checked_headers_;
    static bool checked_query_params_;
    static bool load_entities_;
};

} // namespace xscript

#endif // _XSCRIPT_HTTP_EXTENSION_H_
