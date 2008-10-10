#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/config.h"
#include "xscript/validator_factory.h"
#include "xscript/util.h"

#include "range_validator.h"

using namespace xscript;

class RangeValidatorTest : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(RangeValidatorTest);


    CPPUNIT_TEST_SUITE_END();
private:
};

CPPUNIT_TEST_SUITE_REGISTRATION( RangeValidatorTest );

