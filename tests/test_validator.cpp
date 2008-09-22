#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/validator_factory.h"
#include "xscript/xml_helpers.h"

using namespace xscript;

class ValidatorTest : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(ValidatorTest);
    CPPUNIT_TEST(testCreateFactory);
    CPPUNIT_TEST(testCreateNonexisted);
    CPPUNIT_TEST_SUITE_END();

    void testCreateFactory() {
        ValidatorFactory * factory = ValidatorFactory::instance();
        CPPUNIT_ASSERT(factory);
    }

    void testCreateNonexisted() {
        ValidatorFactory * factory = ValidatorFactory::instance();

        XmlNodeHelper node(xmlNewNode(NULL, BAD_CAST "param"));
        std::auto_ptr<ValidatorBase> val = factory->createValidator(node.get());
    }
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ValidatorTest, "validator");
CPPUNIT_REGISTRY_ADD("validator", "xscript");

