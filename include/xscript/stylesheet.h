#ifndef _XSCRIPT_STYLESHEET_H_
#define _XSCRIPT_STYLESHEET_H_

#include <map>
#include <boost/shared_ptr.hpp>

#include <xscript/xml.h>
#include <xscript/xml_helpers.h>
#include <libxslt/xsltInternals.h>

namespace xscript
{

class Block;
class Param;
class Object;
class Context;
class StylesheetFactory;


class Stylesheet : public Xml
{
public:
	virtual ~Stylesheet();
	virtual time_t modified() const;
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
	
	XmlDocHelper apply(Object *obj, Context *ctx, const XmlDocHelper &doc);

	static Context* getContext(xsltTransformContextPtr tctx);
	static Stylesheet* getStylesheet(xsltTransformContextPtr tctx);
	static boost::shared_ptr<Stylesheet> create(const std::string &name);

	Block* block(xmlNodePtr node);

protected:
	Stylesheet(const std::string &name);
	
	virtual void parse();
	void parseImport(xsltStylesheetPtr imp);
	void parseNode(xmlNodePtr node);

	void detectOutputMethod(const XsltStylesheetHelper &sh);
	void detectOutputEncoding(const XsltStylesheetHelper &sh);
	void detectOutputInfo(const XsltStylesheetHelper &sh);
	
	void setupContentType(const char *type);
	std::string detectContentType(const XmlDocHelper &doc) const;
	
	static void attachContextData(xsltTransformContextPtr tctx, Context *ctx, Stylesheet *stylesheet);

private:

	friend class StylesheetFactory;
	
private:
	time_t modified_;
	std::string name_;
	XsltStylesheetHelper stylesheet_;
	std::map<xmlNodePtr, Block*> blocks_;
	std::string content_type_, output_method_, output_encoding_;
	bool have_output_info_;
};

} // namespace xscript

#endif // _XSCRIPT_STYLESHEET_H_
