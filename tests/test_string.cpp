#include "settings.h"

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/xml_util.h"
#include "xscript/string_utils.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

class StringTest : public CppUnit::TestFixture {
public:
    void testAmp();
    void testEmpty();
    void testEqual();
    void testParams();
    void testEscape();
    void testUrlencode();
    void testUrlencodeEmpty();
    void testUrlencodeLatin();
    void testUrldecode();
    void testUrldecodeEmpty();
    void testUrldecodeLatin();
    void testUrldecodeBadSuffix();

private:
    CPPUNIT_TEST_SUITE(StringTest);
    CPPUNIT_TEST(testAmp);
    CPPUNIT_TEST(testEmpty);
    CPPUNIT_TEST(testEqual);
    CPPUNIT_TEST(testParams);
    CPPUNIT_TEST(testEscape);
    CPPUNIT_TEST(testUrlencode);
    CPPUNIT_TEST(testUrlencodeEmpty);
    CPPUNIT_TEST(testUrlencodeLatin);
    CPPUNIT_TEST(testUrldecode);
    CPPUNIT_TEST(testUrldecodeEmpty);
    CPPUNIT_TEST(testUrldecodeLatin);
    CPPUNIT_TEST(testUrldecodeBadSuffix);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StringTest, "string");
CPPUNIT_REGISTRY_ADD("string", "xscript");

void
StringTest::testAmp() {

    using namespace xscript;
    std::vector<StringUtils::NamedValue> v;

    std::string str("&");
    StringUtils::parse(str, v);
    CPPUNIT_ASSERT_EQUAL(static_cast<std::vector<StringUtils::NamedValue>::size_type>(0), v.size());
}

void
StringTest::testEmpty() {

    using namespace xscript;
    std::vector<StringUtils::NamedValue> v;

    std::string str("");
    StringUtils::parse(str, v);
    CPPUNIT_ASSERT_EQUAL(static_cast<std::vector<StringUtils::NamedValue>::size_type>(0), v.size());
}

void
StringTest::testEqual() {

    using namespace xscript;
    std::vector<StringUtils::NamedValue> v;

    std::string str("=");
    StringUtils::parse(str, v);
    CPPUNIT_ASSERT_EQUAL(static_cast<std::vector<StringUtils::NamedValue>::size_type>(0), v.size());
}

void
StringTest::testEscape() {

    using namespace xscript;

    CPPUNIT_ASSERT_EQUAL(std::string("abcde"), XmlUtils::escape("abcde"));
    CPPUNIT_ASSERT_EQUAL(std::string("johnson&amp;johnson"), XmlUtils::escape("johnson&johnson"));
    CPPUNIT_ASSERT_EQUAL(std::string("&lt;td&gt;test&lt;/test&gt;"), XmlUtils::escape("<td>test</test>"));
    CPPUNIT_ASSERT_EQUAL(std::string("&lt;td colspan=&quot;2&quot;&gt;"), XmlUtils::escape("<td colspan=\"2\">"));
}

void
StringTest::testParams() {

    using namespace xscript;
    std::vector<StringUtils::NamedValue> v;

    std::string str("test=yes&successful=try%20again");
    StringUtils::parse(str, v);

    CPPUNIT_ASSERT_EQUAL(static_cast<std::vector<StringUtils::NamedValue>::size_type>(2), v.size());
    CPPUNIT_ASSERT_EQUAL(std::string("test"), v[0].first);
    CPPUNIT_ASSERT_EQUAL(std::string("yes"), v[0].second);
    CPPUNIT_ASSERT_EQUAL(std::string("successful"), v[1].first);
    CPPUNIT_ASSERT_EQUAL(std::string("try again"), v[1].second);
}

void
StringTest::testUrlencode() {

    using namespace xscript;
    std::string str("раз два три четыре пять"), res = StringUtils::urlencode(str);
    CPPUNIT_ASSERT_EQUAL(std::string("%D2%C1%DA%20%C4%D7%C1%20%D4%D2%C9%20%DE%C5%D4%D9%D2%C5%20%D0%D1%D4%D8"), res);
}

void
StringTest::testUrlencodeEmpty() {

    using namespace xscript;
    std::string str, res = StringUtils::urlencode(str);
    CPPUNIT_ASSERT_EQUAL(std::string(""), res);
}

void
StringTest::testUrlencodeLatin() {

    using namespace xscript;
    std::string str("abcd efgh"), res = StringUtils::urlencode(str);
    CPPUNIT_ASSERT_EQUAL(std::string("abcd%20efgh"), res);
}

void
StringTest::testUrldecode() {

    using namespace xscript;
    std::string str("%D2%C1%DA%20%C4%D7%C1%20%D4%D2%C9%20%DE%C5%D4%D9%D2%C5%20%D0%D1%D4%D8"), res = StringUtils::urldecode(str);
    CPPUNIT_ASSERT_EQUAL(std::string("раз два три четыре пять"), res);

}

void
StringTest::testUrldecodeEmpty() {

    using namespace xscript;
    std::string str, res = StringUtils::urldecode(str);
    CPPUNIT_ASSERT_EQUAL(std::string(""), res);
}

void
StringTest::testUrldecodeLatin() {

    using namespace xscript;
    std::string str("abcd%20efgh"), res = StringUtils::urldecode(str);
    CPPUNIT_ASSERT_EQUAL(std::string("abcd efgh"), res);
}

void
StringTest::testUrldecodeBadSuffix() {

    using namespace xscript;
    std::string str("abcd%20efgh%"), res = StringUtils::urldecode(str);
    CPPUNIT_ASSERT_EQUAL(std::string("abcd efgh%"), res);
}
