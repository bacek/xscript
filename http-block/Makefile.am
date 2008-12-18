pkglib_LTLIBRARIES = xscript-http.la

xscript_http_la_SOURCES = http_helper.cpp http_block.cpp


xscript_http_la_LIBADD = ../library/libxscript.la
xscript_http_la_LDFLAGS = -lcurl -module -export-dynamic @VERSION_INFO@

noinst_HEADERS = http_helper.h http_block.h

AM_CPPFLAGS = -I@top_srcdir@/include -I@builddir@/config -D_REENTRANT @yandex_platform_CFLAGS@
AM_CXXFLAGS = -W -Wall -fexceptions -frtti -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = -lboost_thread -lboost_filesystem @yandex_platform_LIBS@
