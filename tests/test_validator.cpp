#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class ValidatorTest : public CppUnit::TestFixture {

    CPPUNIT_TEST_SUITE(ValidatorTest);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ValidatorTest, "validator");
CPPUNIT_REGISTRY_ADD("validator", "xscript");
