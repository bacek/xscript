#include "settings.h"

#include <exception>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/script.h"
#include "xscript/script_factory.h"
#include "xscript/string_utils.h"

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
    boost::shared_ptr<Script> script = ScriptFactory::createScript("script.xml");

    CPPUNIT_ASSERT_EQUAL(std::string("script.xml"), script->name());
    CPPUNIT_ASSERT_EQUAL(std::string("script.xsl"), script->xsltName());

    CPPUNIT_ASSERT_EQUAL(false, script->forceStylesheet());
    CPPUNIT_ASSERT(script->allowMethod("GET"));

    const std::map<std::string, std::string> &headers = script->headers();
    
    std::map<std::string, std::string>::const_iterator it = headers.find("Expires");
    std::string header = it == headers.end() ? StringUtils::EMPTY_STRING : it->second;
    CPPUNIT_ASSERT_EQUAL(std::string("0"), header);
    
    it = headers.find("Content-type");
    header = it == headers.end() ? StringUtils::EMPTY_STRING : it->second;
    CPPUNIT_ASSERT_EQUAL(std::string("text/html; charset=windows-1251"), header);

#ifdef HAVE_HTTP_BLOCK

    CPPUNIT_ASSERT(NULL != script->block(0));
    CPPUNIT_ASSERT(NULL != script->block("getBanner", false));

#endif

}

void
ScriptTest::testMalformed() {

    using namespace xscript;
    boost::shared_ptr<Script> script = ScriptFactory::createScript("malformed.xml");
}

void
ScriptTest::testBadHeader() {

    using namespace xscript;
    boost::shared_ptr<Script> script = ScriptFactory::createScript("script.xml");
    
    const std::map<std::string, std::string> &headers = script->headers();
    if (headers.end() == headers.find("X-Custom-Header")) {
        throw std::runtime_error("X-Custom-Header is absent");
    }
}

void
ScriptTest::testBadInclude() {

    using namespace xscript;
    boost::shared_ptr<Script> script = ScriptFactory::createScript("badinclude.xml");
}

void
ScriptTest::testNonexistent() {

    using namespace xscript;
    boost::shared_ptr<Script> script = ScriptFactory::createScript("nonexistent.xml");
}

void
ScriptTest::testBadBlockId() {

    using namespace xscript;
    boost::shared_ptr<Script> script = ScriptFactory::createScript("script.xml");
    script->block("nonexistent-id");
}

void
ScriptTest::testBadBlockIndex() {
    using namespace xscript;
    boost::shared_ptr<Script> script = ScriptFactory::createScript("script.xml");
    script->block(256);
}
