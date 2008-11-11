#include "settings.h"

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/validator_factory.h"
#include "xscript/validator_exception.h"
#include "xscript/xml_helpers.h"
#include "xscript/script.h"
#include "xscript/context.h"
#include "xscript/state.h"
#include "xscript/request_data.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

using namespace xscript;

class ValidatorMockup : public xscript::Validator {
public:
    ValidatorMockup(xmlNodePtr node) : Validator(node) {}
    void checkImpl(const Context * /* ctx */, const xscript::Param & /* value */) const {
        return;
    }
};

Validator * createMockup(xmlNodePtr node) {
    return new ValidatorMockup(node);
}

class AlwaysFailValidator : public xscript::Validator {
public:
    AlwaysFailValidator(xmlNodePtr node) : Validator(node) {}
    void checkImpl(const Context * /* ctx */, const xscript::Param & /* value */) const {
        throw ValidatorException("EPIC FAILURE");
    }
};
Validator * createFailed(xmlNodePtr node) {
    return new AlwaysFailValidator(node);
}


class ValidatorTest : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(ValidatorTest);
    CPPUNIT_TEST(testCreateFactory);
    CPPUNIT_TEST(testCreateNoop);
    CPPUNIT_TEST_EXCEPTION(testCreateNonexisted, std::runtime_error);
    CPPUNIT_TEST(testRegisterContructor);
    CPPUNIT_TEST_EXCEPTION(testRegisterContructor, std::runtime_error);
    CPPUNIT_TEST(testConstruct);
    CPPUNIT_TEST(testBlockPass);
    CPPUNIT_TEST(testBlockFail);
    CPPUNIT_TEST_SUITE_END();

    void testCreateFactory() {
        ValidatorFactory * factory = ValidatorFactory::instance();
        CPPUNIT_ASSERT(factory);
    }

    void testCreateNoop() {
        ValidatorFactory * factory = ValidatorFactory::instance();

        XmlNodeHelper node(xmlNewNode(NULL, reinterpret_cast<const xmlChar*>("param")));
        std::auto_ptr<Validator> val = factory->createValidator(node.get());
        CPPUNIT_ASSERT(!val.get());
    }
    
    void testCreateNonexisted() {
        ValidatorFactory * factory = ValidatorFactory::instance();

        XmlNodeHelper node(xmlNewNode(NULL, reinterpret_cast<const xmlChar*>("param")));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("validator"), reinterpret_cast<const xmlChar*>("foo"));
        std::auto_ptr<Validator> val = factory->createValidator(node.get());
    }

    // We use this function twice. On second call it should throw runtime_error
    void testRegisterContructor() {
        ValidatorFactory * factory = ValidatorFactory::instance();
        factory->registerConstructor("foo", &createMockup);
        factory->registerConstructor("bar", &createFailed);
    }

    void testConstruct() {
        ValidatorFactory * factory = ValidatorFactory::instance();
        // We don't have to register constructor. It's already register by
        // previous test

        XmlNodeHelper node(xmlNewNode(NULL, reinterpret_cast<const xmlChar*>("param")));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("validator"), reinterpret_cast<const xmlChar*>("foo"));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("validate-error-guard"), reinterpret_cast<const xmlChar*>("guard"));
        std::auto_ptr<Validator> val = factory->createValidator(node.get());
        CPPUNIT_ASSERT(val.get());

        // Check that we've got our mockup.
        ValidatorMockup * real = dynamic_cast<ValidatorMockup*>(val.get());
        CPPUNIT_ASSERT(real);

        CPPUNIT_ASSERT("guard" == val->guardName());
    }

    // Check that validator created and failed.
    void testBlockFail() {
        boost::shared_ptr<RequestData> data(new RequestData());
        boost::shared_ptr<Script> script = Script::create("./validator.xml");
        boost::shared_ptr<Context> ctx(new Context(script, data));
        ContextStopper ctx_stopper(ctx);

        XmlDocHelper doc(script->invoke(ctx));
        CPPUNIT_ASSERT(NULL != doc.get());
        CPPUNIT_ASSERT(xscript::XmlUtils::xpathExists(doc.get(), "//xscript_invoke_failed"));

        // Guard was set
        CPPUNIT_ASSERT(ctx->state()->is("epic-failure"));
    }

    // Check that validator created and passed.
    void testBlockPass() {
        boost::shared_ptr<RequestData> data(new RequestData());
        boost::shared_ptr<Script> script = Script::create("./validator2.xml");
        boost::shared_ptr<Context> ctx(new Context(script, data));
        ContextStopper ctx_stopper(ctx);

        XmlDocHelper doc(script->invoke(ctx));
        CPPUNIT_ASSERT(NULL != doc.get());
        CPPUNIT_ASSERT(xscript::XmlUtils::xpathExists(doc.get(), "//include-data"));
        
        // Guard wasn't set
        CPPUNIT_ASSERT(!ctx->state()->is("epic-failure"));
    }
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ValidatorTest, "validator");
CPPUNIT_REGISTRY_ADD("validator", "xscript");

