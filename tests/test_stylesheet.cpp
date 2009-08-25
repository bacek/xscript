#include "settings.h"

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/stylesheet.h"
#include "xscript/stylesheet_factory.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

class StylesheetTest : public CppUnit::TestFixture {
public:
    void testInvalid();
    void testNonexistent();
    void testContentType();
    void testOutputMethod();
    void testOutputEncoding();

private:
    CPPUNIT_TEST_SUITE(StylesheetTest);

    CPPUNIT_TEST(testContentType);
    CPPUNIT_TEST(testOutputMethod);
    CPPUNIT_TEST(testOutputEncoding);

    CPPUNIT_TEST_EXCEPTION(testInvalid, std::exception);
    CPPUNIT_TEST_EXCEPTION(testNonexistent, std::exception);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StylesheetTest, "stylesheet");
CPPUNIT_REGISTRY_ADD("stylesheet", "xscript");

void
StylesheetTest::testInvalid() {

    using namespace xscript;
    boost::shared_ptr<Stylesheet> sh = StylesheetFactory::createStylesheet("malformed.xsl");
}

void
StylesheetTest::testNonexistent() {

    using namespace xscript;
    boost::shared_ptr<Stylesheet> sh = StylesheetFactory::createStylesheet("nonexistent.xsl");
}

void
StylesheetTest::testContentType() {

    using namespace xscript;

    boost::shared_ptr<Stylesheet> sh = StylesheetFactory::createStylesheet("stylesheet.xsl");
    CPPUNIT_ASSERT_EQUAL(std::string("text/html"), sh->contentType());

    boost::shared_ptr<Stylesheet> xmlout = StylesheetFactory::createStylesheet("xmlout.xsl");
    CPPUNIT_ASSERT_EQUAL(std::string("text/xml"), xmlout->contentType());

    boost::shared_ptr<Stylesheet> htmlout = StylesheetFactory::createStylesheet("htmlout.xsl");
    CPPUNIT_ASSERT_EQUAL(std::string("text/html"), htmlout->contentType());

    boost::shared_ptr<Stylesheet> textout = StylesheetFactory::createStylesheet("textout.xsl");
    CPPUNIT_ASSERT_EQUAL(std::string("text/plain"), textout->contentType());

    boost::shared_ptr<Stylesheet> custom = StylesheetFactory::createStylesheet("custom.xsl");
    CPPUNIT_ASSERT_EQUAL(std::string("image/swg"), custom->contentType());
}

void
StylesheetTest::testOutputMethod() {

    using namespace xscript;

    boost::shared_ptr<Stylesheet> sh = StylesheetFactory::createStylesheet("stylesheet.xsl");
    CPPUNIT_ASSERT_EQUAL(std::string("html"), sh->outputMethod());

    boost::shared_ptr<Stylesheet> xmlout = StylesheetFactory::createStylesheet("xmlout.xsl");
    CPPUNIT_ASSERT_EQUAL(std::string("xml"), xmlout->outputMethod());

    boost::shared_ptr<Stylesheet> htmlout = StylesheetFactory::createStylesheet("htmlout.xsl");
    CPPUNIT_ASSERT_EQUAL(std::string("html"), htmlout->outputMethod());

    boost::shared_ptr<Stylesheet> textout = StylesheetFactory::createStylesheet("textout.xsl");
    CPPUNIT_ASSERT_EQUAL(std::string("text"), textout->outputMethod());

    boost::shared_ptr<Stylesheet> custom = StylesheetFactory::createStylesheet("custom.xsl");
    CPPUNIT_ASSERT_EQUAL(std::string("xml"), custom->outputMethod());
}

void
StylesheetTest::testOutputEncoding() {

    using namespace xscript;

    boost::shared_ptr<Stylesheet> sh = StylesheetFactory::createStylesheet("stylesheet.xsl");
    CPPUNIT_ASSERT_EQUAL(std::string("utf-8"), sh->outputEncoding());

    boost::shared_ptr<Stylesheet> xmlout = StylesheetFactory::createStylesheet("xmlout.xsl");
    CPPUNIT_ASSERT_EQUAL(std::string("utf-8"), xmlout->outputEncoding());

    boost::shared_ptr<Stylesheet> htmlout = StylesheetFactory::createStylesheet("htmlout.xsl");
    CPPUNIT_ASSERT_EQUAL(std::string("koi8-r"), htmlout->outputEncoding());

    boost::shared_ptr<Stylesheet> textout = StylesheetFactory::createStylesheet("textout.xsl");
    CPPUNIT_ASSERT_EQUAL(std::string("windows-1251"), textout->outputEncoding());

    boost::shared_ptr<Stylesheet> custom = StylesheetFactory::createStylesheet("custom.xsl");
    CPPUNIT_ASSERT_EQUAL(std::string("utf-8"), custom->outputEncoding());
}
