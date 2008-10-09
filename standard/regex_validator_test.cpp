#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/config.h"
#include "xscript/validator_factory.h"
#include "xscript/util.h"

#include "regex_validator.h"

using namespace xscript;

class RegexValidatorTest : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(RegexValidatorTest);

    // If pattern not provided we should throw exception
    CPPUNIT_TEST_EXCEPTION(testAbsentPattern, std::exception);

    CPPUNIT_TEST(testTrivialPatternCompile);
    // If pattern is not correct RE we should throw exception
    CPPUNIT_TEST_EXCEPTION(testWrongPattern, std::exception);

    CPPUNIT_TEST_SUITE_END();

public:
    RegexValidatorTest() {
        ValidatorFactory * factory = ValidatorFactory::instance();
        factory->registerConstructor("regex", &RegexValidator::create);
    }

    void testAbsentPattern() {
    };

    void testTrivialPatternCompile() {
    };

    void testWrongPattern() {
    };
};

CPPUNIT_TEST_SUITE_REGISTRATION( RegexValidatorTest );

