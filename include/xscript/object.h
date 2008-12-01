#ifndef _XSCRIPT_OBJECT_H_
#define _XSCRIPT_OBJECT_H_

#include <string>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <xscript/xml_helpers.h>

namespace xscript {

class Block;
class Param;
class Context;
class Stylesheet;
class ParamFactory;

class Object : private boost::noncopyable {
public:
    Object();
    virtual ~Object();

    inline const std::string& xsltName() const {
        return xslt_name_;
    }

    inline const std::vector<Param*>& xsltParams() const {
        return params_;
    }

    virtual std::string fullName(const std::string &name) const = 0;
    virtual void applyStylesheet(Context *ctx, XmlDocHelper &doc) = 0;

protected:
    virtual void postParse();
    void xsltName(const std::string &value);

    void checkParam(const Param *param) const;
    bool xsltParamNode(const xmlNodePtr node) const;

    void parseXsltParamNode(const xmlNodePtr node, ParamFactory *pf);
    void applyStylesheet(boost::shared_ptr<Stylesheet> sh, Context *ctx, XmlDocHelper &doc, bool need_copy);

private:
    std::string xslt_name_;
    std::vector<Param*> params_;
};

} // namespace xscript

#endif // _XSCRIPT_OBJECT_H_
