if HAVE_TESTS

TESTS = test
check_PROGRAMS = test

test_SOURCES = main.cpp test_config.cpp test_date.cpp test_encoder.cpp \
	test_factories.cpp test_http.cpp test_loader.cpp \
	test_script.cpp test_invoke.cpp test_mist.cpp test_punycode.cpp \
	test_range.cpp test_state.cpp test_string.cpp test_stylesheet.cpp \
	test_xmlcache.cpp test_xslt.cpp \
	test_file.cpp \
	test_doc_cache.cpp \
	test_validator.cpp

test_LDADD = ../library/libxscript.la
test_LDFLAGS = @CPPUNIT_LIBS@ -export-dynamic @BOOST_THREAD_LDFLAGS@ \
	@BOOST_FILESYSTEM_LDFLAGS@ @BOOST_SYSTEM_LDFLAGS@ @yandex_platform_LIBS@

endif

AM_CPPFLAGS = -I@top_srcdir@/include -I@builddir@/config -D_REENTRANT @yandex_platform_CFLAGS@
AM_CXXFLAGS = -W -Wall -fexceptions -frtti -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = @BOOST_THREAD_LDFLAGS@ @BOOST_FILESYSTEM_LDFLAGS@ @yandex_platform_LIBS@

EXTRA_DIST = test.conf badinclude.xml custom.xsl empty-cdata.xml \
	file-include.xml file-load.xml file-unknownMethod.xml htmlout.xsl \
	http-block-params.xml http-block-params.xsl http-get.xml \
	http-local-tagged.xml http-local.xml http-sanitized.xml http-timeout.xml \
	include/block-include.xml include/headers-include.xml \
	include/param-include.xml include.xml include/xscript-include.xml \
	invoke.xml malformed.xml malformed.xsl mist-badmethod.xml mist-date.xml \
	mist-defined.xml mist-domain.xml mist-drop.xml mist-escape.xml \
	mist-extension.xml mist-extension.xsl mist-keys.xml mist-split.xml \
	mist-style.xml mist-types.xml noblocks.xml params.xml params.xsl \
	script.xml stylesheet.xsl textout.xsl validator.xml validator2.xml \
	x-esc.xml x-esc.xsl xinclude.xml x-md5.xml x-md5.xsl xmlout.xsl \
	x-nl2br.xml x-nl2br.xsl x-nodeset.xsl xslt.xml x-tests.xsl \
	x-urldecode.xml x-urldecode.xsl x-urlencode.xml x-urlencode.xsl x-wbr.xml \
	x-wbr.xsl
	
CLEANFILES = default.log
