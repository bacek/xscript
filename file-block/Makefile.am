pkglib_LTLIBRARIES = xscript-file.la

xscript_file_la_SOURCES = file_block.cpp file_extension.cpp


xscript_file_la_LIBADD = ../library/libxscript.la
xscript_file_la_LDFLAGS = -lcurl -module -avoid-version -export-dynamic

noinst_HEADERS = file_block.h file_extension.h

AM_CXXFLAGS = -pthread
AM_CPPFLAGS = -I../include -I../config @yandex_platform_CFLAGS@
AM_LDFLAGS = -lboost_thread -lboost_filesystem @yandex_platform_LIBS@

