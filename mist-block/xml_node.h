#ifndef _XSCRIPT_MIST_XML_NODE_H_
#define _XSCRIPT_MIST_XML_NODE_H_

#include <libxml/tree.h>

namespace xscript
{

class XmlNodeCommon
{
protected:
	XmlNodeCommon();
	virtual ~XmlNodeCommon();

public:
	xmlNodePtr getNode() const;

	void setContent(const char* val);
	void setProperty(const char* name, const char* val);

	void setType(const char* type_str);

private:
	XmlNodeCommon(const XmlNodeCommon &);
	XmlNodeCommon& operator = (const XmlNodeCommon &);

protected:
	xmlNodePtr node_;
};


class XmlNode : public XmlNodeCommon
{
public:
	explicit XmlNode(const char* name);
	virtual ~XmlNode();

	xmlNodePtr releaseNode();
};


class XmlChildNode : public XmlNodeCommon
{
public:
	XmlChildNode(xmlNodePtr parent, const char* name, const char* val);
};

} // namespace xscript

#endif // _XSCRIPT_MIST_XML_NODE_H_
