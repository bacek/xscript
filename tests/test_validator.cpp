#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/validator_factory.h"

using namespace xscript;

class ValidatorTest : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(ValidatorTest);
    CPPUNIT_TEST(testCreate);
    CPPUNIT_TEST_SUITE_END();

    void testCreate() {
        ValidatorFactory * factory = ValidatorFactory::instance();
        CPPUNIT_ASSERT(factory);
    }
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ValidatorTest, "validator");
CPPUNIT_REGISTRY_ADD("validator", "xscript");

