#include "settings.h"

#include <exception>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/script.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

// We use x:http for testing purposes.
#ifdef HAVE_HTTP_BLOCK

class ScriptPageTest : public CppUnit::TestFixture 
{
    CPPUNIT_TEST_SUITE(ScriptPageTest);
    CPPUNIT_TEST(testCalcCacheable);
    CPPUNIT_TEST_SUITE_END();

public:
    void testCalcCacheable();

};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ScriptPageTest, "script_page_cache");
CPPUNIT_REGISTRY_ADD("script_page_cache", "xscript");

void
ScriptPageTest::testCalcCacheable() {

    using namespace xscript;

    boost::shared_ptr<Script> script = Script::create("script/t0.xml");
    CPPUNIT_ASSERT(script->cacheWholePage());

    // Single non-tagged block
    script = Script::create("script/t1.xml");
    CPPUNIT_ASSERT(! script->cacheWholePage());

    // Single tagged block
    script = Script::create("script/t2.xml");
    CPPUNIT_ASSERT(script->cacheWholePage());

    // Mixed taggeg and non-tagged block
    script = Script::create("script/t3.xml");
    CPPUNIT_ASSERT(! script->cacheWholePage());

    // Two tagged block
    script = Script::create("script/t4.xml");
    CPPUNIT_ASSERT(script->cacheWholePage());
    
    // Block with tag-override
    script = Script::create("script/t5.xml");
    CPPUNIT_ASSERT(script->cacheWholePage());
}

#endif
