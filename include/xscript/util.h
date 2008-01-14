#ifndef _XSCRIPT_UTIL_H_
#define _XSCRIPT_UTIL_H_

#include <ctime>
#include <string>
#include <vector>
#include <iosfwd>
#include <boost/cstdint.hpp>
#include <boost/utility.hpp>
#include <xscript/range.h>
#include <libxml/tree.h>

namespace xscript
{

class Encoder;

class XmlUtils : private boost::noncopyable
{
public:
	XmlUtils();
	virtual ~XmlUtils();

	static void registerReporters();
	static void throwUnless(bool value);
	
	static std::string escape(const Range &value);
	template<typename Cont> static std::string escape(const Cont &value);
	
	static std::string sanitize(const Range &value);
	template<typename Cont> static std::string sanitize(const Cont &value);

	template<typename NodePtr> static const char* value(NodePtr node);

	template<typename Visitor> static void visitAttributes(xmlAttrPtr attr, Visitor visitor);
	
	static inline const char* attrValue(xmlNodePtr node, const char *name) {
		xmlAttrPtr attr = xmlHasProp(node, (const xmlChar*) name);
		return attr ? value(attr) : NULL;
	}
	
	static bool xpathExists(xmlDocPtr doc, const std::string &path);
	static std::string xpathValue(xmlDocPtr doc, const std::string &path, const std::string &defval = "");

	static const char * const XSCRIPT_NAMESPACE;
};

class StringUtils : private boost::noncopyable
{
public:
	static const std::string EMPTY_STRING;
	typedef std::pair<std::string, std::string> NamedValue;
	
	static void report(const char *what, int error, std::ostream &stream);
	
	static std::string urlencode(const Range &val);
	template<typename Cont> static std::string urlencode(const Cont &cont);
	
	static std::string urldecode(const Range &val);
	template<typename Cont> static std::string urldecode(const Cont &cont);
	
	static void parse(const Range &range, std::vector<NamedValue> &v, Encoder *encoder = NULL);
	template<typename Cont> static void parse(const Cont &cont, std::vector<NamedValue> &v, Encoder *encoder = NULL);

private:
	StringUtils();
	virtual ~StringUtils();
};

class HttpDateUtils : private boost::noncopyable
{
public:
	static time_t parse(const char *value);
	static std::string format(time_t value);

private:
	HttpDateUtils();
	virtual ~HttpDateUtils();
};

template<typename Cont> inline std::string
XmlUtils::escape(const Cont &value) {
	return escape(createRange(value));
}

template<typename Cont> inline std::string
XmlUtils::sanitize(const Cont &value) {
	return sanitize(createRange(value));
}

template <typename NodePtr> inline const char* 
XmlUtils::value(NodePtr node) {
	xmlNodePtr child = node->children;
	if (child && xmlNodeIsText(child) && child->content) {
		return (const char*) child->content;
	}
	return NULL;
}

template<typename Visitor> inline void
XmlUtils::visitAttributes(xmlAttrPtr attr, Visitor visitor) {
	std::size_t len = strlen(XSCRIPT_NAMESPACE) + 1;
	while (attr) {
		xmlNsPtr ns = attr->ns;
		if (NULL == ns || xmlStrncmp(ns->href, (const xmlChar*) XSCRIPT_NAMESPACE, len) == 0) {
			const char *name = (const char*) attr->name, *val = value(attr);
			if (name && val) {
				visitor(name, val);
			}
		}
		attr = attr->next;
	}
}

template<typename Cont> inline std::string
StringUtils::urlencode(const Cont &cont) {
	return urlencode(createRange(cont));
}
	
template<typename Cont> inline std::string
StringUtils::urldecode(const Cont &cont) {
	return urldecode(createRange(cont));
}

template<typename Cont> inline void
StringUtils::parse(const Cont &cont, std::vector<NamedValue> &v, Encoder *encoder) {
	parse(createRange(cont), v, encoder);
}

} // namespace xscript

#endif // _XSCRIPT_UTIL_H_
