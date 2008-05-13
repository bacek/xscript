#include "settings.h"

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/config.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

class ConfigTest : public CppUnit::TestFixture
{
public:
	void testConfig();
	void testSubkeys();
	void testVariables();

private:
	CPPUNIT_TEST_SUITE(ConfigTest);
	CPPUNIT_TEST(testConfig);
	CPPUNIT_TEST(testSubkeys);
	CPPUNIT_TEST(testVariables);
	CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ConfigTest, "config");
CPPUNIT_REGISTRY_ADD("config", "load");

void
ConfigTest::testConfig() {
	
	using namespace xscript;
	
	std::auto_ptr<Config> config = Config::create("test.conf");
	
	CPPUNIT_ASSERT_EQUAL(10, config->as<int>("/xscript/endpoint/backlog"));
	CPPUNIT_ASSERT_EQUAL(std::string("/tmp/xscript.sock"), config->as<std::string>("/xscript/endpoint/socket"));
	
	std::vector<std::string> v;
	config->subKeys("/xscript/modules/module", v);
	CPPUNIT_ASSERT_EQUAL(static_cast<std::vector<std::string>::size_type>(5), v.size());
	CPPUNIT_ASSERT_EQUAL(std::string("../standard/.libs/xscript-thrpool.so"), config->as<std::string>(v[0] + "/path"));
}

void
ConfigTest::testSubkeys() {
	
	using namespace xscript;
	
	std::vector<std::string> v;
	std::auto_ptr<Config> config = Config::create("test.conf");
	
	config->subKeys("/xscript/script-cache/deny", v);
	CPPUNIT_ASSERT_EQUAL(static_cast<std::vector<std::string>::size_type>(4), v.size());
	CPPUNIT_ASSERT_EQUAL(std::string("/usr/local/www/doc/test-page.xml"), config->as<std::string>(v[1]));
	CPPUNIT_ASSERT_EQUAL(std::string("/usr/local/www/doc/test-search.xml"), config->as<std::string>(v[2]));
}

void
ConfigTest::testVariables() {

	using namespace xscript;
	std::auto_ptr<Config> config = Config::create("test.conf");
	CPPUNIT_ASSERT_EQUAL(std::string("xscript"), config->as<std::string>("/xscript/variables/variable[@name='name']"));
}
