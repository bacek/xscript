#ifndef _XSCRIPT_STYLESHEET_H_
#define _XSCRIPT_STYLESHEET_H_

#include <boost/shared_ptr.hpp>

#include <xscript/xml.h>
#include <xscript/xml_helpers.h>

#include <libxslt/xsltInternals.h>

namespace xscript {

class Block;
class Param;
class Object;
class Context;
class StylesheetFactory;


class Stylesheet : public Xml {
public:
    virtual ~Stylesheet();

    xsltStylesheetPtr stylesheet() const;
    const std::string& contentType() const;
    const std::string& outputMethod() const;
    const std::string& outputEncoding() const;
    bool haveOutputInfo() const;

    XmlDocHelper apply(Object *obj, boost::shared_ptr<Context> ctx, const XmlDocHelper &doc);

    static boost::shared_ptr<Context> getContext(xsltTransformContextPtr tctx);
    static Stylesheet* getStylesheet(xsltTransformContextPtr tctx);
    static const Block* getBlock(xsltTransformContextPtr tctx);
    
    Block* block(xmlNodePtr node);

protected:
    Stylesheet(const std::string &name);

    void parse();

private:
    friend class StylesheetFactory;

    class StylesheetData;
    StylesheetData *data_;
};

} // namespace xscript

#endif // _XSCRIPT_STYLESHEET_H_
