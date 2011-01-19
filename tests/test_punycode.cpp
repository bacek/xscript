#include "settings.h"

#include <string>
#include <fstream>
#include <iterator>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

//#include "xscript/util.h"
#include "xscript/punycode_utils.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

class PunycodeTest : public CppUnit::TestFixture {
public:
    void testEncodeEmpty();
    void testEncode();
    void testDomainEncode();
    void testDecodeEmpty();
    void testDecode();
    void testDomainDecode();

private:
    CPPUNIT_TEST_SUITE(PunycodeTest);
    CPPUNIT_TEST(testEncodeEmpty);
    CPPUNIT_TEST(testEncode);
    CPPUNIT_TEST(testDomainEncode);
    CPPUNIT_TEST(testDecodeEmpty);
    CPPUNIT_TEST(testDecode);
    CPPUNIT_TEST(testDomainDecode);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(PunycodeTest, "punycode");
CPPUNIT_REGISTRY_ADD("punycode", "xscript");

void
PunycodeTest::testEncodeEmpty() {

    using namespace xscript;

    std::string str;
    PunycodeEncoder encoder;
    CPPUNIT_ASSERT_EQUAL(str, encoder.encode(str));
}

void
PunycodeTest::testEncode() {

    using namespace xscript;

    PunycodeEncoder encoder;
    CPPUNIT_ASSERT_EQUAL(std::string("b1agh1afp"), encoder.encode("привет"));
}

void
PunycodeTest::testDomainEncode() {

    using namespace xscript;

    PunycodeEncoder encoder;
    CPPUNIT_ASSERT_EQUAL(std::string("..xn--80a.xn--90a..xn--b1a..xn--c1a.xn--d1a.."), encoder.domainEncode("..а.б..в..г.д.."));
    CPPUNIT_ASSERT_EQUAL(std::string(".www.xn--p1ai"), encoder.domainEncode(".www.рф"));
    CPPUNIT_ASSERT_EQUAL(std::string("www.ru"), encoder.domainEncode("www.ru"));
}

void
PunycodeTest::testDecodeEmpty() {

    using namespace xscript;

    std::string str;
    PunycodeDecoder decoder;
    CPPUNIT_ASSERT_EQUAL(str, decoder.decode(str));
}

void
PunycodeTest::testDecode() {

    using namespace xscript;

    PunycodeDecoder decoder;
    CPPUNIT_ASSERT_EQUAL(std::string("привет"), decoder.decode("b1agh1afp"));
}

void
PunycodeTest::testDomainDecode() {

    using namespace xscript;

    PunycodeDecoder decoder;
    CPPUNIT_ASSERT_EQUAL(std::string("..а.б..в..г.д.."), decoder.domainDecode("..xn--80a.xn--90a..xn--b1a..xn--c1a.xn--d1a.."));
    CPPUNIT_ASSERT_EQUAL(std::string(".www.рф"), decoder.domainDecode(".www.xn--p1ai"));
    CPPUNIT_ASSERT_EQUAL(std::string("www.ru"), decoder.domainDecode("www.ru"));
}
