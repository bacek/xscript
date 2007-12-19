#include "settings.h"

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "details/param_factory.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

class FactoriesTest : public CppUnit::TestFixture
{
public:
	void testParamFactory();
	
private:
	CPPUNIT_TEST_SUITE(FactoriesTest);
	CPPUNIT_TEST(testParamFactory);
	CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(FactoriesTest, "factories");
CPPUNIT_REGISTRY_ADD("factories", "xscript");

void
FactoriesTest::testParamFactory() {

	using namespace xscript;

	ParamFactory *f1 = ParamFactory::instance();
	ParamFactory *f2 = ParamFactory::instance();
	
	CPPUNIT_ASSERT(f1 == f2);
}

