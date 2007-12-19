pkglib_LTLIBRARIES = xscript-http.la

xscript_http_la_SOURCES = http_helper.cpp http_block.cpp


xscript_http_la_LIBADD = ../library/libxscript.la
xscript_http_la_LDFLAGS = -lcurl -module

noinst_HEADERS = http_helper.h http_block.h

AM_CPPFLAGS = -I../include -I../config -D_REENTRANT
AM_CXXFLAGS = -Wall -fexceptions -frtti -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = -lboost_thread -lboost_regex -lboost_filesystem
