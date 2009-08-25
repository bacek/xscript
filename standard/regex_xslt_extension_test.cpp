#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <iostream>

#include "xscript/context.h"
#include "xscript/request_data.h"
#include "xscript/script.h"
#include "xscript/script_factory.h"
#include "xscript/xml_util.h"
#include "xscript/xml_helpers.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

class RegexXsltExtensionTest : public CppUnit::TestFixture {
public:
	void testRegexp();
	void testBadRegexp();

private:
	CPPUNIT_TEST_SUITE(RegexXsltExtensionTest);
	CPPUNIT_TEST(testRegexp);
	CPPUNIT_TEST(testBadRegexp);
	CPPUNIT_TEST_SUITE_END();
};

void
RegexXsltExtensionTest::testRegexp() {

	using namespace xscript;
    boost::shared_ptr<RequestData> data(new RequestData());
	boost::shared_ptr<Script> script = ScriptFactory::createScript("regexp.xml");
	boost::shared_ptr<Context> ctx(new Context(script, data));
	ContextStopper ctx_stopper(ctx);
	XmlDocHelper doc(script->invoke(ctx));
	CPPUNIT_ASSERT(NULL != doc.get());
	script->applyStylesheet(ctx, doc);
	CPPUNIT_ASSERT_EQUAL(std::string("true"), XmlUtils::xpathValue(doc.get(), "/result/status", "failed"));
}

void
RegexXsltExtensionTest::testBadRegexp() {

	using namespace xscript;
    boost::shared_ptr<RequestData> data(new RequestData());
	boost::shared_ptr<Script> script = ScriptFactory::createScript("badregexp.xml");
	boost::shared_ptr<Context> ctx(new Context(script, data));
	ContextStopper ctx_stopper(ctx);
	XmlDocHelper doc(script->invoke(ctx));
	CPPUNIT_ASSERT(NULL != doc.get());
	try {
		script->applyStylesheet(ctx, doc);
	}
	catch (UnboundRuntimeError &e) {
		std::cerr << "\n Detect legacy libxml2 error: " << e.what() << std::endl;
	}
}

CPPUNIT_TEST_SUITE_REGISTRATION(RegexXsltExtensionTest);
