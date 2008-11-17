#ifndef _XSCRIPT_DETAILS_XML_CONFIG_H_
#define _XSCRIPT_DETAILS_XML_CONFIG_H_

#include <string>

#include "xscript/config.h"
#include "xscript/xml_helpers.h"

#include "internal/hash.h"
#include "internal/hashmap.h"

#ifndef HAVE_HASHMAP
#include <map>
#endif

namespace xscript {

#ifndef HAVE_HASHMAP
typedef std::map<std::string, std::string> VarMap;
#else
typedef details::hash_map<std::string, std::string, details::StringHash> VarMap;
#endif

class XmlConfig : public Config {
public:
    XmlConfig(const char *file);
    virtual ~XmlConfig();

    virtual std::string value(const std::string &value) const;
    virtual void subKeys(const std::string &value, std::vector<std::string> &v) const;

private:
    void findVariables(const XmlDocHelper &doc);
    void resolveVariables(std::string &val) const;
    const std::string& findVariable(const std::string &key) const;

private:
    VarMap vars_;
    XmlDocHelper doc_;
};

} // namespace xscript

#endif // _XSCRIPT_DETAILS_XML_CONFIG_H_
