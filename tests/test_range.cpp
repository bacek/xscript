#include "settings.h"

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/range.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

class RangeTest : public CppUnit::TestFixture
{
public:
	void testLess();

private:
	CPPUNIT_TEST_SUITE(RangeTest);
	CPPUNIT_TEST(testLess);
	CPPUNIT_TEST_SUITE_END();
};
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(RangeTest, "range");
CPPUNIT_REGISTRY_ADD("range", "xscript");

void
RangeTest::testLess() {
	
	using namespace xscript;
	
	CPPUNIT_ASSERT(createRange("abcd") < createRange("bcde"));
	CPPUNIT_ASSERT(createRange("abcd") < createRange("abcde"));
}
