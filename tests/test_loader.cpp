#include "settings.h"

#include <cerrno>
#include <climits>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/xml_util.h"
#include "xscript/config.h"
#include "details/loader.h"
#include "xscript/extension.h"
#include "details/extension_list.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

class LoaderTest : public CppUnit::TestFixture {
public:
    void testConfig();
    void testLoader();
    void testLuaLoad();
    void testMistLoad();
    void testHttpLoad();
    void testModuleEqual();
    void testDuplicate();
    void testInexistent();

private:
    CPPUNIT_TEST_SUITE(LoaderTest);
    CPPUNIT_TEST(testLoader);
    CPPUNIT_TEST_EXCEPTION(testInexistent, std::exception);

#ifdef HAVE_HTTP_BLOCK
    CPPUNIT_TEST(testHttpLoad);
#endif

#ifdef HAVE_MIST_BLOCK
    CPPUNIT_TEST(testMistLoad);
    CPPUNIT_TEST(testModuleEqual);
#endif

#ifdef HAVE_LUA_BLOCK
    CPPUNIT_TEST(testLuaLoad);
#endif

    CPPUNIT_TEST_SUITE_END();

private:
    static std::auto_ptr<xscript::Config> config_;
};

std::auto_ptr<xscript::Config> LoaderTest::config_;

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(LoaderTest, "LoaderTest");
CPPUNIT_REGISTRY_ADD("LoaderTest", "load");

void
LoaderTest::testConfig() {

    using namespace xscript;
    boost::shared_ptr<Loader> loader = Loader::instance();
    loader->init(config_.get());
}

void
LoaderTest::testLoader() {

    using namespace xscript;
    boost::shared_ptr<Loader> l1 = Loader::instance();
    boost::shared_ptr<Loader> l2 = Loader::instance();
    CPPUNIT_ASSERT_EQUAL(l1.get(), l2.get());
}

void
LoaderTest::testLuaLoad() {

    using namespace xscript;
    boost::shared_ptr<Loader> loader = Loader::instance();
    loader->load("../lua-block/.libs/xscript-lua.so");
    Extension *ext = ExtensionList::instance()->extension("lua", XmlUtils::XSCRIPT_NAMESPACE);
    CPPUNIT_ASSERT(NULL != ext);
}

void
LoaderTest::testMistLoad() {

    using namespace xscript;
    boost::shared_ptr<Loader> loader = Loader::instance();
    loader->load("../mist-block/.libs/xscript-mist.so");
    Extension *ext = ExtensionList::instance()->extension("mist", XmlUtils::XSCRIPT_NAMESPACE);
    CPPUNIT_ASSERT(NULL != ext);
}

void
LoaderTest::testHttpLoad() {

    using namespace xscript;
    boost::shared_ptr<Loader> loader = Loader::instance();
    loader->load("../http-block/.libs/xscript-http.so");
    Extension *ext = ExtensionList::instance()->extension("http", XmlUtils::XSCRIPT_NAMESPACE);
    CPPUNIT_ASSERT(NULL != ext);
}

void
LoaderTest::testModuleEqual() {

    using namespace xscript;
    boost::shared_ptr<Loader> loader = Loader::instance();
    Extension *e1 = ExtensionList::instance()->extension("mist", XmlUtils::XSCRIPT_NAMESPACE);
    Extension *e2 = ExtensionList::instance()->extension("mist", XmlUtils::XSCRIPT_NAMESPACE);
    CPPUNIT_ASSERT_EQUAL(e1, e2);
}

void
LoaderTest::testInexistent() {
    using namespace xscript;
    boost::shared_ptr<Loader> loader = Loader::instance();
    loader->load("inexistent.so");
}

