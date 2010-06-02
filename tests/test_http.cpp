#include "settings.h"

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/context.h"
#include "xscript/script.h"
#include "xscript/script_factory.h"
#include "xscript/state.h"
#include "xscript/test_utils.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

class HttpTest : public CppUnit::TestFixture {
public:
    void setUp();
    void testGet();
    void testGetLocal();
    void testSanitized();

private:
    CPPUNIT_TEST_SUITE(HttpTest);
    CPPUNIT_TEST(testGet);
    CPPUNIT_TEST(testGetLocal);
    CPPUNIT_TEST_SUITE_END();
};

#ifdef HAVE_HTTP_BLOCK

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(HttpTest, "http");
CPPUNIT_REGISTRY_ADD("http", "xscript");

#endif

void
HttpTest::setUp() {
    using namespace xscript;
}

void
HttpTest::testGet() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("http-get.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());
}

void
HttpTest::testGetLocal() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("http-local.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());
    CPPUNIT_ASSERT(!XmlUtils::xpathExists(doc.get(), "/page/error"));
}

void
HttpTest::testSanitized() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("http-sanitized.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());
}
