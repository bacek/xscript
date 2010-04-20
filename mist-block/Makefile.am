pkglib_LTLIBRARIES = xscript-mist.la

xscript_mist_la_SOURCES = mist_block.cpp mist_worker.cpp xml_node.cpp state_node.cpp state_param_node.cpp \
        state_prefix_node.cpp xslt_extension.cpp


xscript_mist_la_LIBADD = ../library/libxscript.la
xscript_mist_la_LDFLAGS = -module -export-dynamic @VERSION_INFO@

noinst_HEADERS = mist_block.h mist_worker.h state_node.h state_param_node.h \
        state_prefix_node.h xml_node.h

AM_CPPFLAGS = -I@top_srcdir@/include -I@builddir@/config -D_REENTRANT @yandex_platform_CFLAGS@
AM_CXXFLAGS = -W -Wall -fexceptions -frtti -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = @BOOST_THREAD_LDFLAGS@ @BOOST_FILESYSTEM_LDFLAGS@ @yandex_platform_LIBS@
