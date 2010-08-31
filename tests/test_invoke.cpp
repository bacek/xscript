#include "settings.h"

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/context.h"
#include "xscript/request.h"
#include "xscript/request_data.h"
#include "xscript/script.h"
#include "xscript/script_factory.h"
#include "xscript/state.h"
#include "xscript/test_utils.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

class InvokeTest : public CppUnit::TestFixture {
public:
    void testInvoke();
    void testMeta();
    void testFileBlockParams();
    void testHttpBlockScheme();
    void testParams();
    void testNoBlocks();
    void testEmptyCDATA();
    void testEvalXPath();
    void testEvalXPointer();
    void testCheckGuard();
    void testStylesheet();

private:
    CPPUNIT_TEST_SUITE(InvokeTest);
    CPPUNIT_TEST(testInvoke);
    CPPUNIT_TEST(testMeta);
    CPPUNIT_TEST(testFileBlockParams);
    CPPUNIT_TEST(testHttpBlockScheme);
    CPPUNIT_TEST(testParams);
    CPPUNIT_TEST(testNoBlocks);
    CPPUNIT_TEST(testEmptyCDATA);
    CPPUNIT_TEST(testEvalXPath);
    CPPUNIT_TEST(testEvalXPointer);
    CPPUNIT_TEST(testCheckGuard);
    CPPUNIT_TEST(testStylesheet);
    CPPUNIT_TEST_SUITE_END();
};

#ifdef HAVE_MIST_BLOCK

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(InvokeTest, "invoke");
CPPUNIT_REGISTRY_ADD("invoke", "xscript");

#endif

void
InvokeTest::testInvoke() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("invoke.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());
}

void
InvokeTest::testMeta() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("meta.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());

    std::map<std::string, std::string> ns;
    ns.insert(std::make_pair(std::string("g"), std::string("http://www.ya.ru")));

    CPPUNIT_ASSERT_EQUAL(std::string("value0"),
                         XmlUtils::xpathNsValue(doc.get(), "/page/g:rootmeta/param[@name='key0']", ns, "failed"));
    CPPUNIT_ASSERT_EQUAL(std::string("value1"),
                         XmlUtils::xpathNsValue(doc.get(), "/page/g:rootmeta/param[@name='key1']", ns, "failed"));
    CPPUNIT_ASSERT_EQUAL(std::string("value2"),
                         XmlUtils::xpathNsValue(doc.get(), "/page/g:rootmeta/param[@name='key2']", ns, "failed"));
    CPPUNIT_ASSERT(XmlUtils::xpathNsExists(doc.get(), "/page/g:rootmeta/param[@name='elapsed-time']", ns));
    CPPUNIT_ASSERT_EQUAL(std::string("value0"), ctx->state()->asString("key"));
}

void
InvokeTest::testParams() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("params.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());

    CPPUNIT_ASSERT_EQUAL(std::string("params.xsl"), ctx->script()->xsltName());
    ctx->script()->applyStylesheet(ctx, doc);

    CPPUNIT_ASSERT_EQUAL(std::string("success"),
                         XmlUtils::xpathValue(doc.get(), "/result/status", "failed"));
}

void
InvokeTest::testFileBlockParams() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("file-block-params.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());

    CPPUNIT_ASSERT_EQUAL(std::string("success"),
                         XmlUtils::xpathValue(doc.get(), "/result/status", "failed"));
}

void
InvokeTest::testHttpBlockScheme() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("http-block-scheme.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());
    CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "/result/xscript_invoke_failed/@error"));
}

void
InvokeTest::testNoBlocks() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("noblocks.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());
}

void
InvokeTest::testEmptyCDATA() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("empty-cdata.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());
}

void
InvokeTest::testEvalXPath() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("invoke.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());

    CPPUNIT_ASSERT(ctx->state()->has("delim_expr"));
    CPPUNIT_ASSERT(ctx->state()->has("result_expr"));
    CPPUNIT_ASSERT_EQUAL(std::string("string"), ctx->state()->asString("delim_expr"));
    CPPUNIT_ASSERT_EQUAL(std::string("string"), ctx->state()->asString("result_expr"));

    CPPUNIT_ASSERT(ctx->state()->is("test-xpath-bool-true"));
    CPPUNIT_ASSERT(ctx->state()->is("test-xpath-bool-true2"));
    CPPUNIT_ASSERT(!ctx->state()->is("test-xpath-bool-false"));
    CPPUNIT_ASSERT_EQUAL(std::string("0"), ctx->state()->asString("test-xpath-number0"));
    CPPUNIT_ASSERT_EQUAL(std::string("1"), ctx->state()->asString("test-xpath-number1"));
    CPPUNIT_ASSERT_EQUAL(std::string("3"), ctx->state()->asString("test-xpath-number3"));
    CPPUNIT_ASSERT_EQUAL(std::string("4"), ctx->state()->asString("test-xpath-number4"));
    CPPUNIT_ASSERT_EQUAL(std::string("string"), ctx->state()->asString("test-xpath-string"));
}

void
InvokeTest::testEvalXPointer() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("file-load-xpointer.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());

    CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "/page/simple/item/id"));
    CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "/page/dynamic/item/id"));
}

void
InvokeTest::testCheckGuard() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("invoke.xml");
    ContextStopper ctx_stopper(ctx);

    ctx->state()->setString("guardkey", "some value");

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());

    CPPUNIT_ASSERT(!ctx->state()->has("val-2"));
    CPPUNIT_ASSERT_EQUAL(static_cast<boost::int32_t>(3), ctx->state()->asLong("val-3"));
}

void
InvokeTest::testStylesheet() {

    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("xslt.xml");
    ContextStopper ctx_stopper(ctx);

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());

    CPPUNIT_ASSERT_EQUAL(std::string("long"),
                         XmlUtils::xpathValue(doc.get(), "/page/state-results/type", "failed"));
}
