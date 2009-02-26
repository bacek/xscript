#include "settings.h"
#include "xscript/stylesheet.h"
#include "xscript/stylesheet_factory.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

StylesheetFactory::StylesheetFactory() {
}

StylesheetFactory::~StylesheetFactory() {
}

boost::shared_ptr<Stylesheet>
StylesheetFactory::create(const std::string &name) {
    return boost::shared_ptr<Stylesheet>(new Stylesheet(name));
}

// REGISTER_COMPONENT(StylesheetFactory);
static ComponentRegisterer<StylesheetFactory> reg;

} // namespace xscript
