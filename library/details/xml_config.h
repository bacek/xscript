#ifndef _XSCRIPT_DETAILS_XML_CONFIG_H_
#define _XSCRIPT_DETAILS_XML_CONFIG_H_

#include <string>
#include <vector>

#include "xscript/config.h"

namespace xscript {

class XmlConfig : public Config {
public:
    XmlConfig(const char *file);
    virtual ~XmlConfig();

    virtual std::string value(const std::string &value) const;
    virtual void subKeys(const std::string &value, std::vector<std::string> &v) const;

    virtual const std::string& fileName() const;
private:
    class XmlConfigData;
    std::auto_ptr<XmlConfigData> data_;
};

} // namespace xscript

#endif // _XSCRIPT_DETAILS_XML_CONFIG_H_
