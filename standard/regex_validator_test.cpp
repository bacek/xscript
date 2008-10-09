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
    void testAbsentPattern() {
        XmlNodeHelper node(xmlNewNode(NULL, reinterpret_cast<const xmlChar*>("param")));
        RegexValidator::create(node.get());
    };

    void testTrivialPatternCompile() {
        XmlNodeHelper node(xmlNewNode(NULL, reinterpret_cast<const xmlChar*>("param")));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("pattern"), reinterpret_cast<const xmlChar*>("foo"));
        RegexValidator::create(node.get());
    };

    void testWrongPattern() {
        XmlNodeHelper node(xmlNewNode(NULL, reinterpret_cast<const xmlChar*>("param")));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("pattern"), reinterpret_cast<const xmlChar*>("("));
        RegexValidator::create(node.get());
    };
};

CPPUNIT_TEST_SUITE_REGISTRATION( RegexValidatorTest );

