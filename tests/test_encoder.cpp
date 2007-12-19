#include "settings.h"

#include <string>
#include <fstream>
#include <iterator>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/util.h"
#include "xscript/encoder.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

class EncoderTest : public CppUnit::TestFixture
{
public:
	void testEmpty();
	void testLatin();
	void testMixedDefault();
	void testMixedIgnoring();
	void testMixedEscaping();

private:
	CPPUNIT_TEST_SUITE(EncoderTest);
	CPPUNIT_TEST(testEmpty);
	CPPUNIT_TEST(testLatin);
	CPPUNIT_TEST(testMixedDefault);
	CPPUNIT_TEST(testMixedIgnoring);
	CPPUNIT_TEST(testMixedEscaping);
	CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(EncoderTest, "encoder");
CPPUNIT_REGISTRY_ADD("encoder", "xscript");

void
EncoderTest::testEmpty() {

	using namespace xscript;
	
	std::string str;
	std::auto_ptr<Encoder> conv = Encoder::createEscaping("utf-8", "koi8-r");
	CPPUNIT_ASSERT_EQUAL(str, conv->encode(str));
}

void
EncoderTest::testLatin() {
	
	using namespace xscript;
	
	std::string str("12345 abcde");
	std::auto_ptr<Encoder> conv = Encoder::createEscaping("koi8-r", "utf-8");
	CPPUNIT_ASSERT_EQUAL(str, conv->encode(str));
}

void
EncoderTest::testMixedDefault() {

	using namespace xscript;

	std::auto_ptr<Encoder> conv = Encoder::createDefault("utf-8", "cp1251");

	CPPUNIT_ASSERT_EQUAL(std::string("?stone"), conv->encode("¹stone"));
	CPPUNIT_ASSERT_EQUAL(std::string("r?sum?"), conv->encode("résumé"));
	CPPUNIT_ASSERT_EQUAL(std::string("??????"), conv->encode("ტექსტი"));
	CPPUNIT_ASSERT_EQUAL(std::string("deportaci?n"), conv->encode("deportación"));
}

void
EncoderTest::testMixedIgnoring() {

	using namespace xscript;

	std::auto_ptr<Encoder> conv = Encoder::createIgnoring("utf-8", "cp1251");

	CPPUNIT_ASSERT_EQUAL(std::string("stone"), conv->encode("¹stone"));
	CPPUNIT_ASSERT_EQUAL(std::string("rsum"), conv->encode("résumé"));
	CPPUNIT_ASSERT_EQUAL(std::string(""), conv->encode("ტექსტი"));
	CPPUNIT_ASSERT_EQUAL(std::string("deportacin"), conv->encode("deportación"));
}

void
EncoderTest::testMixedEscaping() {

	using namespace xscript;

	std::auto_ptr<Encoder> conv = Encoder::createEscaping("utf-8", "cp1251");
	
	CPPUNIT_ASSERT_EQUAL(std::string("&#185;stone"), conv->encode("¹stone"));
	CPPUNIT_ASSERT_EQUAL(std::string("r&#233;sum&#233;"), conv->encode("résumé"));
	CPPUNIT_ASSERT_EQUAL(std::string("&#4322;&#4308;&#4325;&#4321;&#4322;&#4312;"), conv->encode("ტექსტი"));
	CPPUNIT_ASSERT_EQUAL(std::string("deportaci&#243;n"), conv->encode("deportación"));
}
