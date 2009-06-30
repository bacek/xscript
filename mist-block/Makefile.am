pkglib_LTLIBRARIES = xscript-mist.la

xscript_mist_la_SOURCES = mist_block.cpp


xscript_mist_la_LIBADD = ../library/libxscript.la
xscript_mist_la_LDFLAGS = -module -export-dynamic @VERSION_INFO@

noinst_HEADERS = mist_block.h

AM_CPPFLAGS = -I@top_srcdir@/include -I@builddir@/config -D_REENTRANT @yandex_platform_CFLAGS@
AM_CXXFLAGS = -W -Wall -fexceptions -frtti -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = -lboost_thread -lboost_filesystem @yandex_platform_LIBS@
