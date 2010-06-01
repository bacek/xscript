pkglib_LTLIBRARIES = xscript-local.la

xscript_local_la_SOURCES = local_block.cpp local_extension.cpp local_arg_list.cpp while_block.cpp while_extension.cpp


xscript_local_la_LIBADD = ../library/libxscript.la
xscript_local_la_LDFLAGS = -module -export-dynamic @VERSION_INFO@

noinst_HEADERS = local_block.h local_extension.h local_arg_list.h while_block.h while_extension.h

AM_CPPFLAGS = -I@top_srcdir@/include -I@builddir@/config -D_REENTRANT @yandex_platform_CFLAGS@
AM_CXXFLAGS = -W -Wall -fexceptions -frtti -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = @BOOST_THREAD_LDFLAGS@ @BOOST_FILESYSTEM_LDFLAGS@ @yandex_platform_LIBS@
