#include "settings.h"

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/http_utils.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

class HttpDateTest : public CppUnit::TestFixture {
public:
    void testParse();
    void testFormat();

private:
    CPPUNIT_TEST_SUITE(HttpDateTest);
    CPPUNIT_TEST(testParse);
    CPPUNIT_TEST(testFormat);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(HttpDateTest, "http-date");
CPPUNIT_REGISTRY_ADD("http-date", "xscript");

void
HttpDateTest::testParse() {

    using namespace xscript;

    CPPUNIT_ASSERT_EQUAL(static_cast<time_t>(1170416178), HttpDateUtils::parse("Fri, 02 Feb 2007 11:36:18 GMT"));
    CPPUNIT_ASSERT_EQUAL(static_cast<time_t>(1170416178), HttpDateUtils::parse("Friday, 02-Feb-07 11:36:18 GMT"));
    CPPUNIT_ASSERT_EQUAL(static_cast<time_t>(1170416178), HttpDateUtils::parse("Fri Feb 2 11:36:18 2007"));
}

void
HttpDateTest::testFormat() {

    using namespace xscript;

    time_t now = static_cast<time_t>(1170416178);
    CPPUNIT_ASSERT_EQUAL(std::string("Fri, 02 Feb 2007 11:36:18 GMT"), HttpDateUtils::format(now));

}
