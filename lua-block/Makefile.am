pkglib_LTLIBRARIES = xscript-lua.la

xscript_lua_la_SOURCES = exception.cpp lua_block.cpp request_methods.cpp \
	response_methods.cpp stack.cpp state_methods.cpp cookie_methods.cpp \
	logger_methods.cpp xscript_methods.cpp local_methods.cpp meta_methods.cpp

xscript_lua_la_LIBADD = ../library/libxscript.la
xscript_lua_la_LDFLAGS = @lua_LIBS@ -module -export-dynamic @VERSION_INFO@

noinst_HEADERS = exception.h lua_block.h method_map.h stack.h xscript_methods.h

AM_CPPFLAGS = -I@top_srcdir@/include -I@builddir@/config -D_REENTRANT @lua_CFLAGS@ @yandex_platform_CFLAGS@
AM_CXXFLAGS = -W -Wall -fexceptions -frtti -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = @BOOST_THREAD_LIB@ @BOOST_FILESYSTEM_LDFLAGS@ @yandex_platform_LIBS@

EXTRA_DIST = test.conf \
	lua-badargcount.xml lua-badcode.xml lua-badtype.xml \
	lua-cookie.xml lua-domain.xml lua-encode.xml lua-get-vhost-arg.xml \
	lua-logger.xml lua-md5.xml lua-meta.xml lua-multi.xml lua-print.xml \
	lua-punycode.xml lua-request.xml lua-local.xml \
	lua-response-redirect.xml lua-response.xml \
	lua-state-has.xml lua-state-is.xml lua-state.xml \
	lua-strsplit.xml lua-xmlescape.xml

if HAVE_TESTS

TESTS = test
check_PROGRAMS = test

test_SOURCES = test_main.cpp test_lua.cpp

test_LDADD = ../library/libxscript.la 
test_LDFLAGS = @CPPUNIT_LIBS@ -export-dynamic

endif
