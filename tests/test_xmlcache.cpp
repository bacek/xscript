#include "settings.h"

#include <cstdlib>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/script.h"
#include "xscript/script_cache.h"
#include "xscript/script_factory.h"
#include "xscript/stylesheet.h"
#include "xscript/stylesheet_cache.h"
#include "xscript/stylesheet_factory.h"
#include "xscript/xml.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

class XmlCacheTest : public CppUnit::TestFixture {
public:
    void testErase();
    void testDenied();
    void testExpired();
    void testEvicting();
    void testStoreScript();
    void testStoreStylesheet();

private:
    CPPUNIT_TEST_SUITE(XmlCacheTest);
    CPPUNIT_TEST(testErase);
    CPPUNIT_TEST(testDenied);
    CPPUNIT_TEST(testExpired);
    CPPUNIT_TEST(testEvicting);
    CPPUNIT_TEST(testStoreScript);
    CPPUNIT_TEST(testStoreStylesheet);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(XmlCacheTest, "xml-cache");
CPPUNIT_REGISTRY_ADD("xml-cache", "xscript");

void
XmlCacheTest::testErase() {

    using namespace xscript;

    boost::shared_ptr<Script> script;
    ScriptCache *factory = ScriptCache::instance();
    factory->clear();

    ScriptFactory::createScript("script.xml");
    script = factory->fetch("script.xml");
    CPPUNIT_ASSERT(NULL != script.get());

    factory->erase("script.xml");
    script = factory->fetch("script.xml");
    CPPUNIT_ASSERT(NULL == script.get());
}

void
XmlCacheTest::testDenied() {

    using namespace xscript;

    ScriptCache *factory = ScriptCache::instance();
    factory->clear();

    ScriptFactory::createScript("noblocks.xml");
    boost::shared_ptr<Script> script = factory->fetch("noblocks.xml");
    CPPUNIT_ASSERT(NULL == script.get());
}

void
XmlCacheTest::testExpired() {

    using namespace xscript;

    ScriptCache *factory = ScriptCache::instance();
    factory->clear();

    ScriptFactory::createScript("script.xml");
    boost::shared_ptr<Script> script = factory->fetch("script.xml");
    CPPUNIT_ASSERT(NULL != script.get());

    system("touch script.xml");
    sleep(5);

    boost::shared_ptr<Script> target = factory->fetch("script.xml");
    CPPUNIT_ASSERT(NULL == target.get());

}

void
XmlCacheTest::testEvicting() {
}

void
XmlCacheTest::testStoreScript() {

    using namespace xscript;

    ScriptCache *factory = ScriptCache::instance();
    factory->clear();

    boost::shared_ptr<Script> script = ScriptFactory::createScript("script.xml");
    boost::shared_ptr<Script> target = factory->fetch("script.xml");
    CPPUNIT_ASSERT(NULL != target.get());
}

void
XmlCacheTest::testStoreStylesheet() {

    using namespace xscript;

    StylesheetCache *factory = StylesheetCache::instance();
    factory->clear();

    boost::shared_ptr<Stylesheet> stylesheet = StylesheetFactory::createStylesheet("stylesheet.xsl");
    boost::shared_ptr<Stylesheet> target = factory->fetch("stylesheet.xsl");
    CPPUNIT_ASSERT(NULL != target.get());
}
