#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/validator_factory.h"
#include "xscript/xml_helpers.h"

using namespace xscript;

class ValidatorMockup : public xscript::ValidatorBase {
public:
    ValidatorMockup(xmlNodePtr node) {}
};

ValidatorBase * createMockup(xmlNodePtr node) {
    return new ValidatorMockup(node);
}

class ValidatorTest : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(ValidatorTest);
    CPPUNIT_TEST(testCreateFactory);
    CPPUNIT_TEST(testCreateNoop);
    CPPUNIT_TEST_EXCEPTION(testCreateNonexisted, std::runtime_error);
    CPPUNIT_TEST(testRegisterContructor);
    CPPUNIT_TEST_EXCEPTION(testRegisterContructor, std::runtime_error);
    CPPUNIT_TEST(testConstruct);
    CPPUNIT_TEST_SUITE_END();

    void testCreateFactory() {
        ValidatorFactory * factory = ValidatorFactory::instance();
        CPPUNIT_ASSERT(factory);
    }

    void testCreateNoop() {
        ValidatorFactory * factory = ValidatorFactory::instance();

        XmlNodeHelper node(xmlNewNode(NULL, BAD_CAST "param"));
        std::auto_ptr<ValidatorBase> val = factory->createValidator(node.get());
        CPPUNIT_ASSERT(!val.get());
    }
    
    void testCreateNonexisted() {
        ValidatorFactory * factory = ValidatorFactory::instance();

        XmlNodeHelper node(xmlNewNode(NULL, BAD_CAST "param"));
        xmlNewProp(node.get(), BAD_CAST "validator", BAD_CAST "foo");
        std::auto_ptr<ValidatorBase> val = factory->createValidator(node.get());
    }

    // We use this function twice. On second call it should throw runtime_error
    void testRegisterContructor() {
        ValidatorFactory * factory = ValidatorFactory::instance();
        factory->registerConstructor("foo", &createMockup);
    }

    void testConstruct() {
        ValidatorFactory * factory = ValidatorFactory::instance();
        // We don't have to register constructor. It's already register by
        // previous test

        XmlNodeHelper node(xmlNewNode(NULL, BAD_CAST "param"));
        xmlNewProp(node.get(), BAD_CAST "validator", BAD_CAST "foo");
        std::auto_ptr<ValidatorBase> val = factory->createValidator(node.get());
        CPPUNIT_ASSERT(val.get());

        // Check that we've got our mockup.
        ValidatorMockup * real = dynamic_cast<ValidatorMockup*>(val.get());
        CPPUNIT_ASSERT(real);
    }
    
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ValidatorTest, "validator");
CPPUNIT_REGISTRY_ADD("validator", "xscript");

