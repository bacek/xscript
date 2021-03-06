#ifndef _XSCRIPT_STYLESHEET_FACTORY_H_
#define _XSCRIPT_STYLESHEET_FACTORY_H_

#include <string>
#include <boost/shared_ptr.hpp>
#include <xscript/component.h>

namespace xscript {

class Stylesheet;

class StylesheetFactory : public Component<StylesheetFactory> {
public:
    StylesheetFactory();
    virtual ~StylesheetFactory();

    static boost::shared_ptr<Stylesheet> createStylesheet(const std::string &name);
    
protected:    
    virtual boost::shared_ptr<Stylesheet> create(const std::string &name);
    static boost::shared_ptr<Stylesheet> createWithParse(const std::string &name);
};

} // namespace xscript

#endif // _XSCRIPT_STYLESHEET_FACTORY_H_
