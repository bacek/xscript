#include "settings.h"

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/context.h"
#include "xscript/script.h"
#include "xscript/script_factory.h"
#include "xscript/state.h"
#include "xscript/stylesheet.h"
#include "xscript/test_utils.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

class XsltTest : public CppUnit::TestFixture {
public:
    void testEsc();
    void testJsQuote();
    void testJSONQuote();
    void testUrlencode();
    void testUrldecode();
    void testMD5();
    void testWbr();
    void testNl2br();
    void testMist();
    void testMistAgain();

private:
    void testFile(const std::string &name);

private:
    CPPUNIT_TEST_SUITE(XsltTest);
    CPPUNIT_TEST(testEsc);
    CPPUNIT_TEST(testJsQuote);
    CPPUNIT_TEST(testJSONQuote);
    CPPUNIT_TEST(testUrlencode);
    CPPUNIT_TEST(testUrldecode);
    CPPUNIT_TEST(testMD5);
    CPPUNIT_TEST(testWbr);
    CPPUNIT_TEST(testNl2br);
    CPPUNIT_TEST(testMist);
    CPPUNIT_TEST(testMistAgain);
    CPPUNIT_TEST_SUITE_END();
};
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(XsltTest, "xslt");
CPPUNIT_REGISTRY_ADD("xslt", "xscript");

void
XsltTest::testEsc() {
    testFile("x-esc.xml");
}

void
XsltTest::testJsQuote() {
    testFile("x-js-quote.xml");
}

void
XsltTest::testJSONQuote() {
    testFile("x-json-quote.xml");
}

void
XsltTest::testUrlencode() {
    testFile("x-urlencode.xml");
}

void
XsltTest::testUrldecode() {
    testFile("x-urldecode.xml");
}

void
XsltTest::testMD5() {
    testFile("x-md5.xml");
}

void
XsltTest::testWbr() {
    testFile("x-wbr.xml");
}

void
XsltTest::testNl2br() {
    testFile("x-nl2br.xml");
}

void
XsltTest::testFile(const std::string &name) {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv(name);
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc->get());

    CPPUNIT_ASSERT_EQUAL(true, ctx->script()->forceStylesheet());
    ctx->script()->applyStylesheet(ctx, doc);

    CPPUNIT_ASSERT_EQUAL(std::string("success"),
                         XmlUtils::xpathValue(doc->get(), "/result/status"));
}

void
XsltTest::testMist() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("mist-extension.xml");
    ContextStopper ctx_stopper(ctx);

    CPPUNIT_ASSERT_EQUAL(std::string("mist-extension.xsl"), ctx->xsltName());

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc->get());

    CPPUNIT_ASSERT_EQUAL(std::string("2"), ctx->state()->asString("c"));

    CPPUNIT_ASSERT_EQUAL(true, ctx->script()->forceStylesheet());
    ctx->script()->applyStylesheet(ctx, doc);

    CPPUNIT_ASSERT_EQUAL(std::string("1"), ctx->state()->asString("a"));
    CPPUNIT_ASSERT_EQUAL(std::string("1"), ctx->state()->asString("b"));
    CPPUNIT_ASSERT_EQUAL(std::string("2"), ctx->state()->asString("d"));
}

void
XsltTest::testMistAgain() {
    testMist();
}
