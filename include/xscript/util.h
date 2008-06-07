#ifndef _XSCRIPT_UTIL_H_
#define _XSCRIPT_UTIL_H_

#include <sys/time.h>
#include <ctime>
#include <string>
#include <vector>
#include <iosfwd>
#include <boost/cstdint.hpp>
#include <boost/utility.hpp>
#include <xscript/config.h>
#include <xscript/range.h>
#include <libxml/tree.h>

namespace xscript
{

class UnboundRuntimeError : public std::exception {
public:
	UnboundRuntimeError(const std::string& error) : error_(error) {}
	virtual ~UnboundRuntimeError() throw () {}
	virtual const char* what() const throw () { return error_.c_str(); }
	
private:
	std::string error_;
};

class Encoder;

class XmlUtils : private boost::noncopyable
{
public:
	XmlUtils();
	virtual ~XmlUtils();

	static void init(const Config *config);

	static void registerReporters();
	static void resetReporter();
	static void throwUnless(bool value);
	static void printXMLError();
	
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

	static std::string tolower(const std::string& str);
	static std::string toupper(const std::string& str);

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

class HashUtils : private boost::noncopyable
{
public:
	static std::string hexMD5(const char *key);

private:
	HashUtils();
	virtual ~HashUtils();
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

class TimeoutCounter {
public:
	TimeoutCounter();
	TimeoutCounter(int timeout);
	~TimeoutCounter();

	void reset(int timeout);
	bool unlimited() const;
	bool expired() const;
	int remained() const;

	static const int UNDEFINED_TIME;
private:
	struct timeval init_time_;
	int timeout_;
};

} // namespace xscript

#endif // _XSCRIPT_UTIL_H_
