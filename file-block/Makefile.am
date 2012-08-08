pkglib_LTLIBRARIES = xscript-file.la

xscript_file_la_SOURCES = file_block.cpp file_extension.cpp


xscript_file_la_LIBADD = ../library/libxscript.la
xscript_file_la_LDFLAGS = -lcurl -module -export-dynamic @VERSION_INFO@

noinst_HEADERS = file_block.h file_extension.h

AM_CPPFLAGS = -I@top_srcdir@/include -I@builddir@/config -D_REENTRANT @yandex_platform_CFLAGS@
AM_CXXFLAGS = -W -Wall -fexceptions -frtti -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = @BOOST_THREAD_LIB@ @BOOST_FILESYSTEM_LDFLAGS@ @yandex_platform_LIBS@

