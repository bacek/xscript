#include "settings.h"

#include <exception>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/script.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

class ScriptTest : public CppUnit::TestFixture {
public:
    void testScript();
    void testMalformed();
    void testBadHeader();
    void testBadInclude();
    void testNonexistent();
    void testBadBlockId();
    void testBadBlockIndex();

private:
    CPPUNIT_TEST_SUITE(ScriptTest);
    CPPUNIT_TEST(testScript);
    CPPUNIT_TEST_EXCEPTION(testBadHeader, std::exception);
    CPPUNIT_TEST_EXCEPTION(testMalformed, std::exception);
    CPPUNIT_TEST_EXCEPTION(testBadInclude, std::exception);
    CPPUNIT_TEST_EXCEPTION(testNonexistent, std::exception);
    CPPUNIT_TEST_EXCEPTION(testBadBlockId, std::exception);
    CPPUNIT_TEST_EXCEPTION(testBadBlockIndex, std::exception);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ScriptTest, "script");
CPPUNIT_REGISTRY_ADD("script", "xscript");

void
ScriptTest::testScript() {

    using namespace xscript;
    boost::shared_ptr<Script> script = Script::create("script.xml");

    CPPUNIT_ASSERT_EQUAL(std::string("script.xml"), script->name());
    CPPUNIT_ASSERT_EQUAL(std::string("script.xsl"), script->xsltName());

    CPPUNIT_ASSERT_EQUAL(true, script->forceAuth());
    CPPUNIT_ASSERT_EQUAL(false, script->forceStylesheet());
    CPPUNIT_ASSERT(script->allowMethod("GET"));

    CPPUNIT_ASSERT_EQUAL(std::string("0"), script->header("Expires"));
    CPPUNIT_ASSERT_EQUAL(std::string("text/html; charset=windows-1251"), script->header("Content-type"));

#ifdef HAVE_HTTP_BLOCK

    CPPUNIT_ASSERT(NULL != script->block(0));
    CPPUNIT_ASSERT(NULL != script->block("getBanner", false));

#endif

}

void
ScriptTest::testMalformed() {

    using namespace xscript;
    boost::shared_ptr<Script> script = Script::create("malformed.xml");
}

void
ScriptTest::testBadHeader() {

    using namespace xscript;
    boost::shared_ptr<Script> script = Script::create("script.xml");
    script->header("X-Custom-Header");
}

void
ScriptTest::testBadInclude() {

    using namespace xscript;
    boost::shared_ptr<Script> script = Script::create("badinclude.xml");
}

void
ScriptTest::testNonexistent() {

    using namespace xscript;
    boost::shared_ptr<Script> script = Script::create("nonexistent.xml");
}

void
ScriptTest::testBadBlockId() {

    using namespace xscript;
    boost::shared_ptr<Script> script = Script::create("script.xml");
    script->block("nonexistent-id");
}

void
ScriptTest::testBadBlockIndex() {
    using namespace xscript;
    boost::shared_ptr<Script> script = Script::create("script.xml");
    script->block(256);
}
