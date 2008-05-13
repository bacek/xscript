if HAVE_TESTS

TESTS = test
check_PROGRAMS = test

test_SOURCES = main.cpp test_config.cpp test_date.cpp test_encoder.cpp \
	test_factories.cpp test_http.cpp test_loader.cpp test_lua.cpp \
	test_script.cpp test_invoke.cpp test_mist.cpp test_range.cpp \
	test_state.cpp test_string.cpp test_stylesheet.cpp test_tagged_cache.cpp \
	test_xmlcache.cpp test_xslt.cpp \
	test_file.cpp

test_LDADD = ../library/libxscript.la
test_LDFLAGS = -ldl -lcppunit -export-dynamic

endif

AM_CPPFLAGS = -I../include -I../config -I../library -D_REENTRANT
AM_CXXFLAGS = -Wall -fexceptions -frtti -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = -lboost_thread -lboost_regex -lboost_filesystem

EXTRA_DIST = test.conf badinclude.xml http-get.xml http-local-tagged.xml \
	http-block-params.xml http-block-params.xsl empty-cdata.xml \
	http-local.xml http-sanitized.xml http-timeout.xml include.xml \
	invoke.xml lua-badcode.xml lua-badtype.xml lua-badargcount.xml \
	lua-request.xml lua-state.xml malformed.xml mist-badmethod.xml \
	mist-date.xml mist-drop.xml mist-escape.xml mist-extension.xml \
	mist-split.xml mist-style.xml mist-types.xml noblocks.xml params.xml \
	script.xml x-esc.xml x-urldecode.xml x-urlencode.xml xslt.xml \
	include/block-include.xml include/headers-include.xml \
	include/param-include.xml include/xscript-include.xml \
	file-load.xml

CLEANFILES = tests.log
