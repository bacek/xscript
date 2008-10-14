#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/config.h"
#include "xscript/validator_factory.h"
#include "xscript/util.h"

#include "range_validator.h"

using namespace xscript;

class RangeValidatorTest : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(RangeValidatorTest);

    CPPUNIT_TEST_EXCEPTION(testNoRange, std::exception);
    CPPUNIT_TEST_EXCEPTION(testWrongRange, std::exception);
    CPPUNIT_TEST(testMinOnly);
    CPPUNIT_TEST(testMaxOnly);
    CPPUNIT_TEST(testBothSide);
    
    CPPUNIT_TEST_EXCEPTION(testAutoTypeMissingAs, std::exception);
    CPPUNIT_TEST_EXCEPTION(testAutoTypeUnknownAs, std::exception);
    CPPUNIT_TEST(testAutoType);
    CPPUNIT_TEST(testAutoTypeCaseless);

    CPPUNIT_TEST_SUITE_END();
private:

    void testNoRange() {
        XmlNodeHelper node(xmlNewNode(NULL, reinterpret_cast<const xmlChar*>("param")));
        RangeValidatorBase<int>::create(node.get());
    }
    
    void testWrongRange() {
        XmlNodeHelper node(xmlNewNode(NULL, reinterpret_cast<const xmlChar*>("param")));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("min"), reinterpret_cast<const xmlChar*>("300"));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("max"), reinterpret_cast<const xmlChar*>("200"));
        std::auto_ptr<Validator> val(RangeValidatorBase<int>::create(node.get()));
    }

    void testMinOnly() {
        XmlNodeHelper node(xmlNewNode(NULL, reinterpret_cast<const xmlChar*>("param")));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("min"), reinterpret_cast<const xmlChar*>("100"));
        std::auto_ptr<Validator> val(RangeValidatorBase<int>::create(node.get()));
        RangeValidatorBase<int> * v = dynamic_cast<RangeValidatorBase<int>*>(val.get());

        CPPUNIT_ASSERT(v);
        CPPUNIT_ASSERT(!v->checkString("10"));
        CPPUNIT_ASSERT(v->checkString("100"));
    }

    void testMaxOnly() {
        XmlNodeHelper node(xmlNewNode(NULL, reinterpret_cast<const xmlChar*>("param")));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("max"), reinterpret_cast<const xmlChar*>("100"));
        std::auto_ptr<Validator> val(RangeValidatorBase<int>::create(node.get()));
        RangeValidatorBase<int> * v = dynamic_cast<RangeValidatorBase<int>*>(val.get());

        CPPUNIT_ASSERT(v);
        CPPUNIT_ASSERT(v->checkString("10"));
        CPPUNIT_ASSERT(!v->checkString("100"));
    }

    void testBothSide() {
        XmlNodeHelper node(xmlNewNode(NULL, reinterpret_cast<const xmlChar*>("param")));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("min"), reinterpret_cast<const xmlChar*>("100"));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("max"), reinterpret_cast<const xmlChar*>("200"));
        std::auto_ptr<Validator> val(RangeValidatorBase<int>::create(node.get()));
        RangeValidatorBase<int> * v = dynamic_cast<RangeValidatorBase<int>*>(val.get());

        CPPUNIT_ASSERT(v);
        CPPUNIT_ASSERT(!v->checkString("10"));
        CPPUNIT_ASSERT(v->checkString("100"));
        CPPUNIT_ASSERT(v->checkString("150"));
        CPPUNIT_ASSERT(!v->checkString("200"));
        CPPUNIT_ASSERT(!v->checkString("1200"));
    }

    void testAutoTypeMissingAs() {
        XmlNodeHelper node(xmlNewNode(NULL, reinterpret_cast<const xmlChar*>("param")));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("min"), reinterpret_cast<const xmlChar*>("100"));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("max"), reinterpret_cast<const xmlChar*>("200"));
        std::auto_ptr<Validator> val(createRangeValidator(node.get()));
    }

    void testAutoTypeUnknownAs() {
        XmlNodeHelper node(xmlNewNode(NULL, reinterpret_cast<const xmlChar*>("param")));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("as"), reinterpret_cast<const xmlChar*>("FOO"));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("min"), reinterpret_cast<const xmlChar*>("100"));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("max"), reinterpret_cast<const xmlChar*>("200"));
        std::auto_ptr<Validator> val(createRangeValidator(node.get()));
    }

    void testAutoType() {
        XmlNodeHelper node(xmlNewNode(NULL, reinterpret_cast<const xmlChar*>("param")));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("as"), reinterpret_cast<const xmlChar*>("int"));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("min"), reinterpret_cast<const xmlChar*>("100"));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("max"), reinterpret_cast<const xmlChar*>("200"));
        std::auto_ptr<Validator> val(createRangeValidator(node.get()));
        RangeValidatorBase<int> * v = dynamic_cast<RangeValidatorBase<int>*>(val.get());

        CPPUNIT_ASSERT(v);
    }

    void testAutoTypeCaseless() {
        XmlNodeHelper node(xmlNewNode(NULL, reinterpret_cast<const xmlChar*>("param")));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("as"), reinterpret_cast<const xmlChar*>("INT"));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("min"), reinterpret_cast<const xmlChar*>("100"));
        xmlNewProp(node.get(), reinterpret_cast<const xmlChar*>("max"), reinterpret_cast<const xmlChar*>("200"));
        std::auto_ptr<Validator> val(createRangeValidator(node.get()));
        RangeValidatorBase<int> * v = dynamic_cast<RangeValidatorBase<int>*>(val.get());

        CPPUNIT_ASSERT(v);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( RangeValidatorTest );

