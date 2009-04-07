#ifndef _XSCRIPT_DETAILS_EXTENSION_LIST_H_
#define _XSCRIPT_DETAILS_EXTENSION_LIST_H_

#include <memory>
#include <vector>

#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

#include <libxml/tree.h>

#include "xscript/extension.h"
#include "internal/phoenix_singleton.h"

namespace xscript {

class Loader;
class Config;
class Context;

class ExtensionList :
            private boost::noncopyable,
            public PhoenixSingleton<ExtensionList> {
public:
    ExtensionList();
    virtual ~ExtensionList();

    void initContext(Context *ctx);
    void stopContext(Context *ctx);
    void destroyContext(Context *ctx);

    void init(const Config *config);

    void registerExtension(ExtensionHolder ext);

    Extension* extension(const xmlNodePtr node, bool allow_empty_namespace) const;
    Extension* extension(const char *name, const char *ref, bool allow_empty_namespace) const;
    
    bool checkScriptProperty(const char *prop, const char *value);

private:
    friend class std::auto_ptr<ExtensionList>;
    bool accepts(Extension *ext, const char *name, const char *ref, bool allow_empty_namespace) const;

private:
    std::vector<Extension*> extensions_;
};

} // namespace xscript

#endif // _XSCRIPT_DETAILS_EXTENSION_LIST_H_
