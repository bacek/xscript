#include "settings.h"

#include <cctype>
#include <cstdarg>
#include <cassert>
#include <stdexcept>

#include <boost/thread/tss.hpp>
#include <boost/current_function.hpp>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlerror.h>

#include <libxslt/xsltutils.h>
#include <libxslt/xsltInternals.h>

#include <libexslt/exslt.h>
#include <libexslt/exsltconfig.h>

#include "xscript/util.h"
#include "xscript/range.h"
#include "xscript/logger.h"
#include "xscript/encoder.h"
#include "xscript/sanitizer.h"
#include "internal/algorithm.h"
#include "xscript/xml_helpers.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

static const Range ESCAPE_PATTERN = createRange("&'\"<>");
const char * const XmlUtils::XSCRIPT_NAMESPACE = "http://www.yandex.ru/xscript";

class XmlErrorReporter
{
public:
	XmlErrorReporter();
	
	void reset();
	const std::string& message() const;
	
	void report(const char *format, ...);
	void report(const char *format, va_list args);
	
	static XmlErrorReporter* reporter();
	
private:
	bool error_;
	std::string message_;
	
	static boost::thread_specific_ptr<XmlErrorReporter> reporter_;
	static const std::string UNKNOWN_XML_ERROR;
};

const std::string StringUtils::EMPTY_STRING;
boost::thread_specific_ptr<XmlErrorReporter> XmlErrorReporter::reporter_;
const std::string XmlErrorReporter::UNKNOWN_XML_ERROR = "unknown XML error";

extern "C" void xmlNullError(void *, const char *, ...);
extern "C" void xmlReportPlainError(void *, const char *, ...);
extern "C" void xmlReportStructuredError(void *, xmlErrorPtr error);

XmlUtils::XmlUtils() {

	xmlInitParser();
	xmlLineNumbersDefault(1);

	xmlLoadExtDtdDefaultValue |= XML_DETECT_IDS;
	xmlLoadExtDtdDefaultValue |= XML_COMPLETE_ATTRS;
	xmlSubstituteEntitiesDefault(1); // 0

	xsltMaxDepth = 40;

	exsltStrRegister();
	exsltDynRegister();
	
	exsltMathRegister();
	exsltSetsRegister();
	exsltFuncRegister();
	
	exsltDateRegister();
	exsltSaxonRegister();
	exsltCommonRegister();

	registerReporters();
}

XmlUtils::~XmlUtils() {
	xsltCleanupGlobals();
	xmlCleanupParser();
}

void
XmlUtils::registerReporters() {
	//xmlSetGenericErrorFunc(NULL, &xmlNullError);
	xmlSetGenericErrorFunc(NULL, &xmlReportPlainError);
	xsltSetGenericErrorFunc(NULL, &xmlReportPlainError);
	//xmlSetStructuredErrorFunc(NULL, &xmlReportStructuredError);
}

void
XmlUtils::throwUnless(bool value) {
	if (!value) {
		XmlErrorReporter *rep = XmlErrorReporter::reporter();
		unbound_runtime_error e(rep->message());
		rep->reset();
		throw e;
	}
}

std::string
XmlUtils::escape(const Range &range) {

	std::string dest;
	dest.reserve(range.size());
	
	Range::const_iterator begin = range.begin(), end = range.end();
	Range::const_iterator pb = ESCAPE_PATTERN.begin(), pe = ESCAPE_PATTERN.end();
	
	while (true) {
		Range::const_iterator pos = std::find_first_of(begin, end, pb, pe);
		if (end == pos) {
			dest.append(begin, end);
			break;
		}
		dest.append(begin, pos);
		switch (*pos) {
			case '<': 
				dest.append("&lt;");
				break;
			case '>':
				dest.append("&gt;");
				break;

			case '"':
				dest.append("&quot;");
				break; 
			case '\'':  {
				dest.append("&apos;");
			}
			case '&': {
				dest.append("&amp;");
				break;
			}
			default:
				dest.push_back(*pos);
				break;
		}
		begin = pos + 1;
	}
	return dest;
}

std::string
XmlUtils::sanitize(const Range &range) {
	return Sanitizer::instance()->sanitize(range);
}

bool
XmlUtils::xpathExists(xmlDocPtr doc, const std::string &path) {
	
	XmlXPathContextHelper ctx(xmlXPathNewContext(doc));
	throwUnless(NULL != ctx.get());
	
	XmlXPathObjectHelper object(xmlXPathEvalExpression((const xmlChar*) path.c_str(), ctx.get()));
	throwUnless(NULL != object.get());
	return (NULL != object->nodesetval && 0 != object->nodesetval->nodeNr);
}

std::string
XmlUtils::xpathValue(xmlDocPtr doc, const std::string &path, const std::string &defval) {

	std::string res = defval;
	XmlXPathContextHelper ctx(xmlXPathNewContext(doc));
	throwUnless(NULL != ctx.get());
	
	XmlXPathObjectHelper object(xmlXPathEvalExpression((const xmlChar*) path.c_str(), ctx.get()));
	throwUnless(NULL != object.get());
	
	if (NULL != object->nodesetval && 0 != object->nodesetval->nodeNr) {
		const char *val = value(object->nodesetval->nodeTab[0]);
		if (NULL != val) {
			res.assign(val);
		}
	}
	return res;
}

std::string
StringUtils::urlencode(const Range &range) {
	
	std::string result;
	result.reserve(3 * range.size());
	
	for (const char* i = range.begin(), *end = range.end(); i != end; ++i) {
		char symbol = (*i);
		if (isalnum(symbol)) {
			result.append(1, symbol);
			continue;
		}
		switch (symbol) {
			case '-': case '_': case '.': case '!': case '~': 
			case '*': case '(': case ')': case '\'': 
				result.append(1, symbol);
				break;
			default:
				result.append(1, '%');
				char bytes[3] = { 0, 0, 0 };
				bytes[0] = (symbol & 0xF0) / 16 ;
				bytes[0] += (bytes[0] > 9) ? 'A' - 10 : '0';
				bytes[1] = symbol & 0x0F;
				bytes[1] += (bytes[1] > 9) ? 'A' - 10 : '0';
				result.append(bytes, sizeof(bytes) - 1);
				break;
		}
	}
	return result;
}

std::string
StringUtils::urldecode(const Range &range) {
	
	std::string result;
	result.reserve(range.size());
	
	for (const char *i = range.begin(), *end = range.end(); i != end; ++i) {
		switch (*i) {
			case '+': 
				result.append(1, ' ');
				break;
			case '%':
				if (std::distance(i, end) > 2) {
					int digit;
					char f = *(i + 1), s = *(i + 2);
					digit = (f >= 'A' ? ((f & 0xDF) - 'A') + 10 : (f - '0')) * 16;
					digit += (s >= 'A') ? ((s & 0xDF) - 'A') + 10 : (s - '0');
					result.append(1, static_cast<char>(digit));
					i += 2;
				}
				else {
					result.append(1, '%');
				}
				break;
			default:
				result.append(1, (*i));
				break;
		}
	}
	return result;
}
	
void
StringUtils::parse(const Range &range, std::vector<NamedValue> &v, Encoder *encoder) {
	
	Range part = range, key, value, head, tail;
	std::auto_ptr<Encoder> enc(NULL);
	if (NULL == encoder) {
		enc = Encoder::createDefault("cp1251", "utf-8");
		encoder = enc.get();
	}
	while (!part.empty()) {
		splitFirstOf(part, "&;", head, tail);
		split(head, '=', key, value);
		if (!key.empty()) {
			try {
				std::pair<std::string, std::string> p(urldecode(key), urldecode(value));
				if (!xmlCheckUTF8((const xmlChar*) p.first.c_str())) {
					encoder->encode(p.first).swap(p.first);
				}
				if (!xmlCheckUTF8((const xmlChar*) p.second.c_str())) {
					encoder->encode(p.second).swap(p.second);
				}
				v.push_back(p);
			}
			catch (const std::exception &e) {
				log()->error("%s, caught exception: %s", e.what());
			}
		}
		part = tail;
	}
}

void
StringUtils::report(const char *what, int error, std::ostream &stream) {
	char buf[256];
	char *msg = strerror_r(error, buf, sizeof(buf));
	stream << what << msg;
}

HttpDateUtils::HttpDateUtils() {
}

HttpDateUtils::~HttpDateUtils() {
}

static const char httpDateUtilsShortWeekdays [7] [4] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
static const char httpDateUtilsShortMonths [12] [4] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

std::string
HttpDateUtils::format(time_t value) {

	struct tm ts;
	memset(&ts, 0, sizeof(struct tm));

	if (NULL != gmtime_r(&value, &ts)) {
		char buf[255];

		//int res = strftime(buf, sizeof(buf), "%a, %d %b %Y %T GMT", &ts);
		int res = snprintf(buf, sizeof(buf) - 1, "%s, %02d %s %04d %02d:%02d:%02d GMT",
			httpDateUtilsShortWeekdays[ts.tm_wday],
			ts.tm_mday, httpDateUtilsShortMonths[ts.tm_mon], ts.tm_year + 1900,
			ts.tm_hour, ts.tm_min, ts.tm_sec);

		if (0 < res) {
			return std::string(buf, buf + res);
		}
	}
	throw std::runtime_error("format date failed");
}

time_t
HttpDateUtils::parse(const char *value) {
	
	struct tm ts;
	memset(&ts, 0, sizeof(struct tm));
	
	const char *formats[] = { "%a, %d %b %Y %T GMT", "%A, %d-%b-%y %T GMT", "%a %b %d %T %Y" };
	for (unsigned int i = 0; i < sizeof(formats)/sizeof(const char*); ++i) {
		if (NULL != strptime(value, formats[i], &ts)) {
			return mktime(&ts) - timezone;
		}
	}
	return static_cast<time_t>(0);
}

XmlErrorReporter::XmlErrorReporter() :
	error_(false)
{}

void
XmlErrorReporter::reset() {
	error_ = false;
	message_.clear();
}

const std::string&
XmlErrorReporter::message() const {
	return error_ ? message_ : UNKNOWN_XML_ERROR;
}

void
XmlErrorReporter::report(const char *format, ...) {
	va_list args;
	va_start(args, format);
	report(format, args);
	log()->xmllog(format, args);
	va_end(args);
}

void
XmlErrorReporter::report(const char *format, va_list args) {
	char part_message[5120];
	vsnprintf(part_message, sizeof(part_message), format, args);
	part_message[5119] = 0;
	message_ += part_message;
	error_ = true;
}

XmlErrorReporter*
XmlErrorReporter::reporter() {
	XmlErrorReporter *rep = reporter_.get();
	if (NULL == rep) {
		rep = new XmlErrorReporter();
		reporter_.reset(rep);
	}
	return rep;
}



extern "C" void
xmlNullError(void *ctx, const char *format, ...) {
}

extern "C" void
xmlReportPlainError(void *ctx, const char *format, ...) {
	try {
		va_list args;
		va_start(args, format);
		XmlErrorReporter::reporter()->report(format, args);
		va_end(args);
	}
	catch (const std::exception &e) {
		log()->error("%s, caught exception: %s", BOOST_CURRENT_FUNCTION, e.what());
	}
}

extern "C" void
xmlReportStructuredError(void *ctx, xmlErrorPtr error) {
	try {
		const char *message = error->message ? error->message : "unknown XML error";
		XmlErrorReporter::reporter()->report("%s line %d", message, error->line);
	}
	catch (const std::exception &e) {
		log()->error("%s, caught exception: %s", BOOST_CURRENT_FUNCTION, e.what());
	}
}

static XmlUtils utils_;

} // namespace xscript
