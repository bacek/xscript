#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/context.h"
#include "xscript/script.h"
#include "xscript/script_factory.h"
#include "xscript/state.h"
#include "xscript/test_utils.h"
#include "xscript/xml_util.h"
#include "xscript/xml_helpers.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

class XsltExtensionTest : public CppUnit::TestFixture {
public:
    void testToLower();
    void testToUpper();

private:
    CPPUNIT_TEST_SUITE(XsltExtensionTest);
    CPPUNIT_TEST(testToLower);
    CPPUNIT_TEST(testToUpper);
    CPPUNIT_TEST_SUITE_END();
};

void
XsltExtensionTest::testToLower() {

    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("x-tolower.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());
    ctx->script()->applyStylesheet(ctx, doc);
    CPPUNIT_ASSERT_EQUAL(std::string("success"), XmlUtils::xpathValue(doc.get(), "/result/status", "failed"));
}

void
XsltExtensionTest::testToUpper() {

    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("x-toupper.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());
    ctx->script()->applyStylesheet(ctx, doc);
    CPPUNIT_ASSERT_EQUAL(std::string("success"), XmlUtils::xpathValue(doc.get(), "/result/status", "failed"));
}

CPPUNIT_TEST_SUITE_REGISTRATION(XsltExtensionTest);
