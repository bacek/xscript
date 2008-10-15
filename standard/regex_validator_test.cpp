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
    CPPUNIT_TEST_EXCEPTION(testEmptyPattern, std::exception);

    CPPUNIT_TEST(testTrivialPatternCompile);
    // If pattern is not correct RE we should throw exception
    CPPUNIT_TEST_EXCEPTION(testWrongPattern, std::exception);

    // Now, real testing
    CPPUNIT_TEST(testIsPassed);
    CPPUNIT_TEST(testUTF8);

    CPPUNIT_TEST_EXCEPTION(testWrongOptions, std::exception);
    CPPUNIT_TEST(testOptionsCaseless);

    CPPUNIT_TEST_SUITE_END();

public:
    void testAbsentPattern() {
        XmlNodeHelper node(xmlNewNode(NULL, reinterpret_cast<const xmlChar*>("param")));
        RegexValidator::create(node.get());
    };
    
    void testEmptyPattern() {
        XmlNodeHelper node(xmlNewNode(NULL, reinterpret_cast<const xmlChar*>("param")));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("pattern"), reinterpret_cast<const xmlChar*>(""));
        std::auto_ptr<Validator> val(RegexValidator::create(node.get()));
    };

    void testTrivialPatternCompile() {
        XmlNodeHelper node(xmlNewNode(NULL, reinterpret_cast<const xmlChar*>("param")));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("pattern"), reinterpret_cast<const xmlChar*>("foo"));
        std::auto_ptr<Validator> val(RegexValidator::create(node.get()));
    };

    void testWrongPattern() {
        XmlNodeHelper node(xmlNewNode(NULL, reinterpret_cast<const xmlChar*>("param")));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("pattern"), reinterpret_cast<const xmlChar*>("("));
        RegexValidator::create(node.get());
    };
    
    void testIsPassed() {
        XmlNodeHelper node(xmlNewNode(NULL, reinterpret_cast<const xmlChar*>("param")));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("pattern"), reinterpret_cast<const xmlChar*>("foo"));
        std::auto_ptr<Validator> val(RegexValidator::create(node.get()));
        RegexValidator * v = dynamic_cast<RegexValidator*>(val.get());
        CPPUNIT_ASSERT(!v->checkString("bar"));
        CPPUNIT_ASSERT(v->checkString("foo"));
    };

    void testUTF8() {
        XmlNodeHelper node(xmlNewNode(NULL, reinterpret_cast<const xmlChar*>("param")));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("pattern"), reinterpret_cast<const xmlChar*>("Превед"));
        std::auto_ptr<Validator> val(RegexValidator::create(node.get()));
        RegexValidator * v = dynamic_cast<RegexValidator*>(val.get());
        CPPUNIT_ASSERT(!v->checkString("медвед"));
        CPPUNIT_ASSERT(v->checkString("Превед"));
    };

    void testWrongOptions() {
        XmlNodeHelper node(xmlNewNode(NULL, reinterpret_cast<const xmlChar*>("param")));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("pattern"), reinterpret_cast<const xmlChar*>("foo"));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("options"), reinterpret_cast<const xmlChar*>("Z"));
        RegexValidator::create(node.get());
    }
    
    void testOptionsCaseless() {
        XmlNodeHelper node(xmlNewNode(NULL, reinterpret_cast<const xmlChar*>("param")));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("pattern"), reinterpret_cast<const xmlChar*>("foo"));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("options"), reinterpret_cast<const xmlChar*>("i"));
        std::auto_ptr<Validator> val(RegexValidator::create(node.get()));
        RegexValidator * v = dynamic_cast<RegexValidator*>(val.get());
        CPPUNIT_ASSERT(v->checkString("FOO"));
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( RegexValidatorTest );

