#include "settings.h"

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/context.h"
#include "xscript/script.h"
#include "xscript/script_factory.h"
#include "xscript/state.h"
#include "xscript/test_utils.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

class MistTest : public CppUnit::TestFixture {
public:
    void testDrop();
    void testTypes();
    void testDate();
    void testSplit();
    void testEscape();
    void testEncode();
    void testBadMethod();
    void testStylesheet();
    void testDefined();
    void testDomain();
    void testKeys();

private:
    CPPUNIT_TEST_SUITE(MistTest);
    CPPUNIT_TEST(testDrop);
    CPPUNIT_TEST(testTypes);
    CPPUNIT_TEST(testDate);
    CPPUNIT_TEST(testSplit);
    CPPUNIT_TEST(testEscape);
    CPPUNIT_TEST(testEncode);
    CPPUNIT_TEST(testStylesheet);
    CPPUNIT_TEST(testDefined);
    CPPUNIT_TEST(testDomain);
    CPPUNIT_TEST(testKeys);
    CPPUNIT_TEST_EXCEPTION(testBadMethod, std::exception);
    CPPUNIT_TEST_SUITE_END();
};

#ifdef HAVE_MIST_BLOCK

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MistTest, "mist");
CPPUNIT_REGISTRY_ADD("mist", "xscript");

#endif

void
MistTest::testDrop() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("mist-drop.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc->get());

    std::vector<std::string> v;
    ctx->state()->keys(v);
    CPPUNIT_ASSERT_EQUAL(static_cast<std::vector<std::string>::size_type>(2), v.size());
}

void
MistTest::testTypes() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("mist-types.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc->get());

    CPPUNIT_ASSERT_EQUAL(static_cast<boost::int32_t>(1), ctx->state()->asLong("val-1"));
    CPPUNIT_ASSERT_EQUAL(std::string("string-2"), ctx->state()->asString("val-2"));
    CPPUNIT_ASSERT_EQUAL(static_cast<double>(3), ctx->state()->asDouble("val-3"));
    CPPUNIT_ASSERT_EQUAL(static_cast<boost::int64_t>(4), ctx->state()->asLongLong("val-4"));

    CPPUNIT_ASSERT_EQUAL(static_cast<boost::int64_t>(4), ctx->state()->asLongLong("prefdef"));
    CPPUNIT_ASSERT_EQUAL(static_cast<boost::int32_t>(3), ctx->state()->asLong("city_code"));
}

void
MistTest::testDate() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("mist-date.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc->get());

    time_t now = time(NULL);

    char buf[31];
    struct tm ttm;
    localtime_r(&now, &ttm);
    strftime(buf, 30, "%Y-%m-%d", &ttm);
    std::string now_str(buf);

    CPPUNIT_ASSERT_EQUAL(now_str, ctx->state()->asString("current_date"));
}

void
MistTest::testSplit() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("mist-split.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc->get());

    CPPUNIT_ASSERT_EQUAL(std::string("test"), ctx->state()->asString("pref0"));
    CPPUNIT_ASSERT_EQUAL(std::string("test string property"), ctx->state()->asString("testconcat"));
    CPPUNIT_ASSERT_EQUAL(std::string("testd string prodperty"), ctx->state()->asString("testjoin"));

}

void
MistTest::testEscape() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("mist-escape.xml");
    ContextStopper ctx_stopper(ctx);

    ctx->state()->setString("data", "<stress>&data;</stress>");

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc->get());
}

void
MistTest::testEncode() {
}

void
MistTest::testStylesheet() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("mist-style.xml");
    ContextStopper ctx_stopper(ctx);

    CPPUNIT_ASSERT_EQUAL(std::string("object.xsl"), ctx->xsltName());

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc->get());

    CPPUNIT_ASSERT_EQUAL(std::string("stylesheet.xsl"), ctx->xsltName());
}


void
MistTest::testDefined() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("mist-defined.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc->get());
    CPPUNIT_ASSERT_EQUAL(std::string("15"), ctx->state()->asString("replace_var"));
}

void
MistTest::testDomain() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("mist-domain.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc->get());
    CPPUNIT_ASSERT_EQUAL(std::string("net"), ctx->state()->asString("tld"));
    CPPUNIT_ASSERT_EQUAL(std::string("localhost"), ctx->state()->asString("no_level"));
    CPPUNIT_ASSERT_EQUAL(std::string("www.yandex.ru"), ctx->state()->asString("no_level_no_scheme"));
    CPPUNIT_ASSERT_EQUAL(std::string("yandex.ru"), ctx->state()->asString("yandex.ru"));
    CPPUNIT_ASSERT_EQUAL(std::string("yandex.ru"), ctx->state()->asString("no_scheme"));
    CPPUNIT_ASSERT_EQUAL(std::string("localhost"), ctx->state()->asString("localhost"));
    CPPUNIT_ASSERT(!ctx->state()->has("invalid"));
    CPPUNIT_ASSERT(!ctx->state()->has("localfile"));
    CPPUNIT_ASSERT(!ctx->state()->has("empty"));
}

void
MistTest::testKeys() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("mist-keys.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc->get());
    CPPUNIT_ASSERT_EQUAL(std::string("value3"), ctx->state()->asString("var"));
}


void
MistTest::testBadMethod() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("mist-badmethod.xml");
}
