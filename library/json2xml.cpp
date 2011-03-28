#include "settings.h"
#include "xscript/json2xml.h"

#include "xscript/encoder.h"
#include "xscript/xml_util.h"


#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

Json2Xml::Json2Xml() {
}

Json2Xml::~Json2Xml() {
}

bool
Json2Xml::enabled() const {
    return false;
}

XmlNodeHelper
Json2Xml::convert(const Range &range, int flags) const {
    (void) range;
    (void) flags;

    return XmlNodeHelper();
}

XmlNodeHelper
Json2Xml::convert(const std::string &str_utf8, int flags) const {
    (void) str_utf8;
    (void) flags;

    return XmlNodeHelper();
}

XmlNodeHelper
Json2Xml::convert(std::istream &input, int flags) const {
    (void) input;
    (void) flags;

    return XmlNodeHelper();
}

XmlDocHelper
Json2Xml::createDoc(const std::string &str_utf8, int flags) const {

    XmlNodeHelper node(convert(str_utf8, flags));
    if (NULL == node.get()) {
        return XmlDocHelper();
    }

    XmlDocHelper result(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != result.get());

    xmlDocSetRootElement(result.get(), node.release());
    return result;
}

XmlDocHelper
Json2Xml::createDoc(const std::string &str, const char *encoding, int flags) const {

    if (NULL == encoding || !strcasecmp(encoding, "utf-8")) {
        return createDoc(str, flags);
    }

    if (!enabled()) {
        return XmlDocHelper();
    }

    std::auto_ptr<Encoder> enc = Encoder::createEscaping(encoding, "utf-8");
    return createDoc(enc->encode(str), flags);
}

static ComponentRegisterer<Json2Xml> reg;

} // namespace xscript
