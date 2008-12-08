bin_PROGRAMS = xscript-bin

xscript_bin_SOURCES = server_request.cpp fcgi_server.cpp main.cpp uptime_counter.cpp

xscript_bin_LDADD = ../library/libxscript.la
xscript_bin_LDFLAGS = -lfcgi -lfcgi++ -export-dynamic

noinst_HEADERS = server_request.h fcgi_server.h uptime_counter.h

dist_sysconf_DATA = xscript.conf.example xscript-ulimits.conf

AM_CXXFLAGS = -pthread
AM_CPPFLAGS = -I../include -I../config @yandex_platform_CFLAGS@
AM_LDFLAGS = -lboost_thread -lboost_filesystem @yandex_platform_LIBS@
