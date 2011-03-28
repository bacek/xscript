#ifndef _XSCRIPT_JSON2XML_H_
#define _XSCRIPT_JSON2XML_H_

#include <string>
#include <istream>

#include <xscript/range.h>
#include <xscript/component.h>
#include <xscript/xml_helpers.h>

namespace xscript {

class Json2Xml : public Component<Json2Xml> {
public:
    Json2Xml();
    virtual ~Json2Xml();

    enum {
        JSON_NONE = 0,
        JSON_ALLOW_COMMENTS = 1,
        JSON_PRINT_TYPE = 2,
        JSON_ALL = JSON_ALLOW_COMMENTS | JSON_PRINT_TYPE,
    };

    virtual bool enabled() const;

    virtual XmlNodeHelper convert(const Range &range, int flags = JSON_ALL) const;
    virtual XmlNodeHelper convert(const std::string &str_utf8, int flags = JSON_ALL) const;
    virtual XmlNodeHelper convert(std::istream &input, int flags = JSON_ALL) const;

    XmlDocHelper createDoc(const std::string &str_utf8, int flags = JSON_ALL) const;
    XmlDocHelper createDoc(const std::string &str, const char *encoding, int flags = JSON_ALL) const;
};

} // namespace xscript

#endif // _XSCRIPT_JSON2XML_H_
