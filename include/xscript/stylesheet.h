#ifndef _XSCRIPT_STYLESHEET_H_
#define _XSCRIPT_STYLESHEET_H_

#include <map>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

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
    virtual const std::string& name() const;

    inline xsltStylesheetPtr stylesheet() const {
        return stylesheet_.get();
    }

    inline const std::string& contentType() const {
        return content_type_;
    }

    inline const std::string& outputMethod() const {
        return output_method_;
    }

    inline const std::string& outputEncoding() const {
        return output_encoding_;
    }

    inline bool haveOutputInfo() const {
        return have_output_info_;
    }

    XmlDocHelper apply(Object *obj, boost::shared_ptr<Context> ctx, const XmlDocHelper &doc);

    static boost::shared_ptr<Context> getContext(xsltTransformContextPtr tctx);
    static Stylesheet* getStylesheet(xsltTransformContextPtr tctx);
    static const Block* getBlock(xsltTransformContextPtr tctx);
    
    Block* block(xmlNodePtr node);

protected:
    Stylesheet(const std::string &name);

    virtual void parse();
    void parseStylesheet(xsltStylesheetPtr style);
    void parseNode(xmlNodePtr node);

    void detectOutputMethod(const XsltStylesheetHelper &sh);
    void detectOutputEncoding(const XsltStylesheetHelper &sh);
    void detectOutputInfo(const XsltStylesheetHelper &sh);

    void setupContentType(const char *type);
    std::string detectContentType(const XmlDocHelper &doc) const;

    void appendXsltParams(const std::vector<Param*>& params, const Context *ctx, xsltTransformContextPtr tctx);

    static void attachContextData(xsltTransformContextPtr tctx, boost::shared_ptr<Context> ctx, Stylesheet *stylesheet, const Block *block);

private:
    friend class StylesheetFactory;

private:
    std::string name_;
    XsltStylesheetHelper stylesheet_;
    std::map<xmlNodePtr, Block*> blocks_;
    std::string content_type_, output_method_, output_encoding_;
    bool have_output_info_;
};

} // namespace xscript

#endif // _XSCRIPT_STYLESHEET_H_
