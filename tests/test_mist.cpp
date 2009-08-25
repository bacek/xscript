#include "settings.h"

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/context.h"
#include "xscript/request_data.h"
#include "xscript/script.h"
#include "xscript/script_factory.h"
#include "xscript/state.h"

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

    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = ScriptFactory::createScript("mist-drop.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    std::vector<std::string> v;
    State* state = ctx->state();
    state->keys(v);
    CPPUNIT_ASSERT_EQUAL(static_cast<std::vector<std::string>::size_type>(2), v.size());
}

void
MistTest::testTypes() {

    using namespace xscript;

    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = ScriptFactory::createScript("mist-types.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    State* state = ctx->state();

    CPPUNIT_ASSERT_EQUAL(static_cast<boost::int32_t>(1), state->asLong("val-1"));
    CPPUNIT_ASSERT_EQUAL(std::string("string-2"), state->asString("val-2"));
    CPPUNIT_ASSERT_EQUAL(static_cast<double>(3), state->asDouble("val-3"));
    CPPUNIT_ASSERT_EQUAL(static_cast<boost::int64_t>(4), state->asLongLong("val-4"));

    CPPUNIT_ASSERT_EQUAL(static_cast<boost::int64_t>(4), state->asLongLong("prefdef"));
    CPPUNIT_ASSERT_EQUAL(static_cast<boost::int32_t>(3), state->asLong("city_code"));
}

void
MistTest::testDate() {

    using namespace xscript;

    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = ScriptFactory::createScript("mist-date.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    State* state = ctx->state();

    time_t now = time(NULL);

    char buf[31];
    struct tm ttm;
    localtime_r(&now, &ttm);
    strftime(buf, 30, "%Y-%m-%d", &ttm);
    std::string now_str(buf);

    CPPUNIT_ASSERT_EQUAL(now_str, state->asString("current_date"));
}

void
MistTest::testSplit() {

    using namespace xscript;

    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = ScriptFactory::createScript("mist-split.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    State* state = ctx->state();

    CPPUNIT_ASSERT_EQUAL(std::string("test"), state->asString("pref0"));
    CPPUNIT_ASSERT_EQUAL(std::string("test string property"), state->asString("testconcat"));
    CPPUNIT_ASSERT_EQUAL(std::string("testd string prodperty"), state->asString("testjoin"));

}

void
MistTest::testEscape() {

    using namespace xscript;

    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = ScriptFactory::createScript("mist-escape.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    State* state = ctx->state();
    state->setString("data", "<stress>&data;</stress>");

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());
}

void
MistTest::testEncode() {
}

void
MistTest::testStylesheet() {

    using namespace xscript;

    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = ScriptFactory::createScript("mist-style.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    CPPUNIT_ASSERT_EQUAL(std::string("object.xsl"), ctx->xsltName());

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    CPPUNIT_ASSERT_EQUAL(std::string("stylesheet.xsl"), ctx->xsltName());
}


void
MistTest::testDefined() {

    using namespace xscript;

    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = ScriptFactory::createScript("mist-defined.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    State* state = ctx->state();

    CPPUNIT_ASSERT_EQUAL(std::string("15"), state->asString("replace_var"));
}

void
MistTest::testDomain() {

    using namespace xscript;

    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = ScriptFactory::createScript("mist-domain.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    State* state = ctx->state();

    CPPUNIT_ASSERT_EQUAL(std::string("net"), state->asString("tld"));
    CPPUNIT_ASSERT_EQUAL(std::string("localhost"), state->asString("no_level"));
    CPPUNIT_ASSERT_EQUAL(std::string("www.yandex.ru"), state->asString("no_level_no_scheme"));
    CPPUNIT_ASSERT_EQUAL(std::string("yandex.ru"), state->asString("yandex.ru"));
    CPPUNIT_ASSERT_EQUAL(std::string("yandex.ru"), state->asString("no_scheme"));
    CPPUNIT_ASSERT_EQUAL(std::string("localhost"), state->asString("localhost"));
    CPPUNIT_ASSERT(!state->has("invalid"));
    CPPUNIT_ASSERT(!state->has("localfile"));
    CPPUNIT_ASSERT(!state->has("empty"));
}

void
MistTest::testKeys() {

    using namespace xscript;

    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = ScriptFactory::createScript("mist-keys.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    State* state = ctx->state();

    CPPUNIT_ASSERT_EQUAL(std::string("value3"), state->asString("var"));
}


void
MistTest::testBadMethod() {

    using namespace xscript;
    boost::shared_ptr<Script> script = ScriptFactory::createScript("mist-badmethod.xml");
}
