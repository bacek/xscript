#include "settings.h"

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/xml_util.h"
#include "xscript/state.h"
#include "xscript/script.h"
#include "xscript/request.h"
#include "xscript/context.h"
#include "xscript/request_data.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

class InvokeTest : public CppUnit::TestFixture {
public:
    void testInvoke();
    void testHttpBlockParams();
    void testParams();
    void testNoBlocks();
    void testEmptyCDATA();
    void testEvalXPath();
    void testCheckGuard();
    void testStylesheet();
    void testRemoveStylesheet();

private:
    CPPUNIT_TEST_SUITE(InvokeTest);
    CPPUNIT_TEST(testInvoke);
    CPPUNIT_TEST(testHttpBlockParams);
    CPPUNIT_TEST(testParams);
    CPPUNIT_TEST(testNoBlocks);
    CPPUNIT_TEST(testEmptyCDATA);
    CPPUNIT_TEST(testEvalXPath);
    CPPUNIT_TEST(testCheckGuard);
    CPPUNIT_TEST(testStylesheet);
    CPPUNIT_TEST(testRemoveStylesheet);
    CPPUNIT_TEST_SUITE_END();
};

#ifdef HAVE_MIST_BLOCK

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(InvokeTest, "invoke");
CPPUNIT_REGISTRY_ADD("invoke", "xscript");

#endif

void
InvokeTest::testInvoke() {

    using namespace xscript;

    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = Script::create("invoke.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());
}

void
InvokeTest::testParams() {

    using namespace xscript;

    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = Script::create("params.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    CPPUNIT_ASSERT_EQUAL(std::string("params.xsl"), script->xsltName());
    script->applyStylesheet(ctx, doc);

    CPPUNIT_ASSERT_EQUAL(std::string("success"),
                         XmlUtils::xpathValue(doc.get(), "/result/status", "failed"));
}

void
InvokeTest::testHttpBlockParams() {

    using namespace xscript;

    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = Script::create("http-block-params.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    CPPUNIT_ASSERT_EQUAL(std::string("success"),
                         XmlUtils::xpathValue(doc.get(), "/result/status", "failed"));
}

void
InvokeTest::testNoBlocks() {

    using namespace xscript;

    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = Script::create("noblocks.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());
}

void
InvokeTest::testEmptyCDATA() {

    using namespace xscript;

    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = Script::create("empty-cdata.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());
}

void
InvokeTest::testEvalXPath() {

    using namespace xscript;

    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = Script::create("invoke.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    State* state = ctx->state();

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    CPPUNIT_ASSERT(state->has("delim_expr"));
    CPPUNIT_ASSERT(state->has("result_expr"));
    CPPUNIT_ASSERT_EQUAL(std::string("string"), state->asString("delim_expr"));
    CPPUNIT_ASSERT_EQUAL(std::string("string"), state->asString("result_expr"));
}

void
InvokeTest::testCheckGuard() {

    using namespace xscript;

    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = Script::create("invoke.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    State* state = ctx->state();
    state->setString("guardkey", "some value");

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    CPPUNIT_ASSERT(!state->has("val-2"));
    CPPUNIT_ASSERT_EQUAL(static_cast<boost::int32_t>(3), state->asLong("val-3"));
}

void
InvokeTest::testStylesheet() {

    using namespace xscript;

    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = Script::create("xslt.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    CPPUNIT_ASSERT_EQUAL(std::string("long"),
                         XmlUtils::xpathValue(doc.get(), "/page/state-results/type", "failed"));
}

void
InvokeTest::testRemoveStylesheet() {

    using namespace xscript;

    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = Script::create("invoke.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    script->removeUnusedNodes(doc);
}
