pkglib_LTLIBRARIES = xscript-file.la

xscript_file_la_SOURCES = file_block.cpp file_extension.cpp


xscript_file_la_LIBADD = ../library/libxscript.la
xscript_file_la_LDFLAGS = -lcurl -module -avoid-version 

noinst_HEADERS = file_block.h file_extension.h

AM_CPPFLAGS = -I@top_srcdir@/include -I@builddir@/config -D_REENTRANT
AM_CXXFLAGS = -Wall -fexceptions -frtti -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = -lboost_thread -lboost_regex -lboost_filesystem

