noinst_LTLIBRARIES = libpcre.la

libpcre_la_SOURCES = get.cpp pcre++.cpp replace.cpp search.cpp split.cpp

libpcre_la_LDFLAGS = -static

AM_CPPFLAGS = -I@top_srcdir@/include -I@builddir@/config -D_REENTRANT @yandex_platform_CFLAGS@
AM_CXXFLAGS = -W -Wall -fexceptions -frtti -fPIC -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = @yandex_platform_LIBS@

noinst_HEADERS = pcre++.h