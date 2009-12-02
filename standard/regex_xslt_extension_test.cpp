#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/context.h"
#include "xscript/exception.h"
#include "xscript/script.h"
#include "xscript/script_factory.h"
#include "xscript/state.h"
#include "xscript/test_utils.h"
#include "xscript/xml_helpers.h"
#include "xscript/xml_util.h"

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
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("regexp.xml");
    ContextStopper ctx_stopper(ctx);
	XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
	CPPUNIT_ASSERT(NULL != doc->get());
	ctx->script()->applyStylesheet(ctx, doc);
	CPPUNIT_ASSERT_EQUAL(std::string("true"), XmlUtils::xpathValue(doc->get(), "/result/status", "failed"));
}

void
RegexXsltExtensionTest::testBadRegexp() {

	using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("badregexp.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc->get());
	try {
	    ctx->script()->applyStylesheet(ctx, doc);
	}
	catch (UnboundRuntimeError &e) {
		std::cerr << "\n Detect legacy libxml2 error: " << e.what() << std::endl;
	}
}

CPPUNIT_TEST_SUITE_REGISTRATION(RegexXsltExtensionTest);
