pkglib_LTLIBRARIES = xscript-http.la

xscript_http_la_SOURCES = http_block.cpp http_extension.cpp query_params.cpp


xscript_http_la_LIBADD = ../library/libxscript.la
xscript_http_la_LDFLAGS = -module -export-dynamic @VERSION_INFO@

noinst_HEADERS = http_block.h http_extension.h query_params.h

AM_CPPFLAGS = -I@top_srcdir@/include -I@builddir@/config -D_REENTRANT @yandex_platform_CFLAGS@
AM_CXXFLAGS = -W -Wall -fexceptions -frtti -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = @BOOST_THREAD_LIB@ @BOOST_FILESYSTEM_LDFLAGS@ @yandex_platform_LIBS@
