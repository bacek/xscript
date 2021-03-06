AC_PREREC(2.5.3)

AC_INIT([xscript], m4_esyscmd([head -n 1 debian/changelog | sed -e 's/.*[(]//' | sed -e 's/[-].*//' | tr -d '\n']), [highpower@yandex-team.ru])
AM_INIT_AUTOMAKE

AC_GNU_SOURCE
AC_CONFIG_HEADERS(config/settings.h)
AC_CONFIG_MACRO_DIR(config)

AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_LIBTOOL

AC_LANG([C++])
AM_MAINTAINER_MODE

AC_CHECK_LIB([idn11], [main])

AX_SET_VERSION_INFO(AC_PACKAGE_VERSION)

PKG_CHECK_MODULES(yplatform, [yandex-platform], [], [AC_MSG_NOTICE([yandex platform configuration not found])])

LIBS="$LIBS $yplatform_LIBS"
CPPFLAGS="$CPPFLAGS $yplatform_CFLAGS -pthread"

AX_BOOST_BASE
AX_BOOST_THREAD
AX_BOOST_FILESYSTEM
AX_BOOST_SYSTEM

AM_ICONV
AM_ICONV_LINK

AC_SYS_LARGEFILE

AC_CHECK_FUNCS(memcmp, [], [])
AC_CHECK_FUNCS(memchr, [], [])
AC_CHECK_FUNCS(memmem, [], [])

ac_have_boost="yes"
ac_stored_libs=$LIBS
ac_stored_cppflags=$CPPFLAGS
ac_stored_cxxflags=$CXXFLAGS

LIBS="$ac_stored_libs -lboost_thread -lboost_filesystem $yandex_platform_LIBS"
CPPFLAGS="$ac_stored_cppflags -pthread $yandex_platform_CFLAGS -pthread"
CXXFLAGS="$ac_stored_cxxflags -W -Wall -pthread"

AC_SUBST(yandex_platform_LIBS)
AC_SUBST(yandex_platform_CFLAGS)

CPPFLAGS=$ac_stored_cppflags
LIBS=$ac_stored_libs

ac_have_fcgi="yes"
ac_stored_libs=$LIBS

LIBS="$ac_stored_libs -lfcgi -lfcgi++"

AC_CHECK_LIB(fcgi, FCGX_Accept_r, [], [ ac_have_fcgi="no" ])
AC_MSG_CHECKING([trying to link with libfcgi++.so])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <fcgio.h>]], [[ char buf[256]; fcgi_streambuf stbuf(buf, sizeof(buf)); ]])],
	[AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no); ac_have_fcgi="no"])

LIBS=$ac_stored_libs

if test "f$ac_have_fcgi" != "fyes"; then
	AC_MSG_ERROR([libfcgi++ not found])
fi

PKG_CHECK_MODULES(xml,
	[libxml-2.0], [], [ AC_MSG_ERROR([libxml-2.0 not found]) ])
PKG_CHECK_MODULES(xslt, 
	[libxslt], [], [ AC_MSG_ERROR([libxslt not found]) ])
PKG_CHECK_MODULES(exslt, 
	[libexslt], [], [ AC_MSG_ERROR([libexslt not found]) ])

LIBS="$LIBS $xml_LIBS $xslt_LIBS $exslt_LIBS $yandex_platform_LIBS -lcrypto"
CPPFLAGS="$CPPFLAGS $xml_CFLAGS $xslt_CFLAGS $exslt_CFLAGS $yandex_platform_CFLABS"

AC_CHECK_HEADER([pcre.h], [ac_have_pcre="yes"; LIBS="$LIBS -lpcre"], [ac_have_pcre="no"])
AC_ARG_ENABLE(xslt-regex, 
	AS_HELP_STRING(--enable-xslt-regex,enables regex xslt extensions compilation),
	[
		if test "f$enableval" = "fyes" && test "f$ac_have_pcre" = "fno" ; then
			AC_MSG_ERROR([pcre not found])
		fi
	])
AM_CONDITIONAL(HAVE_PCRE, [test "f$ac_have_pcre" = "fyes"])

PKG_CHECK_MODULES(glib, [glib-2.0], [ac_have_glib="yes"], [ac_have_glib="no"])
AC_ARG_ENABLE(xslt-glib, 
	AS_HELP_STRING(--enable-xslt-glib,enables glib xslt extensions compilation),
	[
		if test "f$enableval" = "fyes" && test "f$ac_have_glib" = "fno" ; then
			AC_MSG_ERROR([glib not found])
		fi
	])
AM_CONDITIONAL(HAVE_GLIB, [test "f$ac_have_glib" = "fyes"])
if test "f$ac_have_glib" = "fyes"; then
    LIBS="$LIBS $glib_LIBS"
    CPPFLAGS="$CPPFLAGS $glib_CFLAGS"
fi

AM_CONDITIONAL(HAVE_XSLT_STANDARD, test "f$ac_have_pcre" = "fyes" || test "f$ac_have_glib" = "fyes")

ac_have_lua="yes"
ac_lua_found="no"

AC_ARG_ENABLE(lua, 
	AS_HELP_STRING(--enable-lua,enables lua block compilation),
	[
		if test "f$enableval" = "fno"; then
			ac_have_lua="no"
		fi
	])

if test "f$ac_have_lua" = "fyes"; then
	for i in lua lua5.1; do
		PKG_CHECK_MODULES(lua, [$i], [ac_lua_found="yes"], [ false ])
	done
fi

if test "f$ac_lua_found" = "fyes"; then
	AC_SUBST(lua_LIBS)
	AC_SUBST(lua_CFLAGS)
	AC_DEFINE(HAVE_LUA_BLOCK, 1, [Define to 1 if you want to enable lua block compilation.])
else
	AC_MSG_RESULT([lua not found. compilation of lua-block disabled])
fi

AM_CONDITIONAL(HAVE_LUA_BLOCK, [test "f$ac_lua_found" = "fyes"])

ac_have_mist="yes"

AC_ARG_ENABLE(mist, 
	AS_HELP_STRING(--enable-mist,enables mist block compilation),
	[
		if test "f$enableval" = "fno"; then
			ac_have_mist="no"
		fi
	])

if test "f$ac_have_mist" = "fyes"; then
	AC_DEFINE(HAVE_MIST_BLOCK, 1, [Define to 1 if you want to enable mist block compilation.])
fi

AM_CONDITIONAL(HAVE_MIST_BLOCK, [test "f$ac_have_mist" = "fyes"])

ac_have_http="yes"
ac_curl_found="no"

AC_ARG_ENABLE(http, 
	AS_HELP_STRING(--enable-http,enables http block compilation),
	[
		if test "f$enableval" = "fno"; then
			ac_have_http="no"
		fi
	])

if test "f$ac_have_http" = "fyes"; then
	AC_CHECK_LIB(curl, curl_global_init, [ ac_curl_found="yes" ], [])
fi

if test "f$ac_curl_found" = "fyes"; then
	AC_DEFINE(HAVE_HTTP_BLOCK, 1, [Define to 1 if you want to enable http block compilation.])
else
	AC_MSG_RESULT([libcurl not found. compilation of http-block disabled])
fi

AM_CONDITIONAL(HAVE_HTTP_BLOCK, [test "f$ac_curl_found" = "fyes"])	

ac_have_file="yes"

AC_ARG_ENABLE(file, 
	AS_HELP_STRING(--enable-file,enables file block compilation),
	[
		if test "f$enableval" = "fno"; then
			ac_have_file="no"
		fi
	])

if test "f$ac_have_file" = "fyes"; then
	AC_DEFINE(HAVE_FILE_BLOCK, 1, [Define to 1 if you want to enable file block compilation.])
fi

AM_CONDITIONAL(HAVE_FILE_BLOCK, [test "f$ac_have_file" = "fyes"])

AC_CHECK_HEADER([libmemcached/memcached.h], [ac_have_memcache="yes"], [ac_have_memcache="no"])
AM_CONDITIONAL(HAVE_MEMCACHE, [test "f$ac_have_memcache" = "fyes"])	

ac_python="yes"
AC_ARG_ENABLE(python,
	AS_HELP_STRING(--enable-python,enables python module compilation),
	[
		if test "f$enableval" = "fno"; then
			ac_python="no"
		fi
	])
if test "f$ac_python" = "fyes"; then
	AM_PATH_PYTHON
	AC_PYTHON_DEVEL
fi
AM_CONDITIONAL(HAVE_PYTHON, [test "f$ac_python" = "fyes"])

ac_have_dmalloc="no"
ac_dmalloc_found="no"

AC_ARG_ENABLE(dmalloc, 
	AS_HELP_STRING(--enable-dmalloc,builds xscript with dmalloc support),
	[
		if test "f$enableval" = "fyes"; then
			ac_have_dmalloc="yes"
		fi			
	])

if test "f$ac_have_dmalloc" = "fyes"; then
	ac_dmalloc_found="yes"
	AC_CHECK_HEADER([dmalloc.h], [], [ac_dmalloc_found="no"])
	AC_CHECK_LIB(dmallocthcxx, dmalloc_shutdown, [], [ac_dmalloc_found="no"])
fi

if test "f$ac_dmalloc_found" = "fyes"; then
	LIBS="$LIBS -ldmallocthcxx"
	CPPFLAGS="$CPPFLAGS -DDMALLOC_FUNC_CHECK"
	AC_DEFINE(HAVE_DMALLOC_H, 1, [Define to 1 if you have the <dmalloc.h> header file])
else
	AC_MSG_RESULT([dmalloc not found. dmalloc check disabled])
fi


AX_CHECK_CPPUNIT

AC_ARG_ENABLE(coverage,
	AS_HELP_STRING(--enable-coverage,enables coverage tests compilation),
	[
		if test "f$enableval" = "fyes"; then
			CXXFLAGS="$CXXFLAGS --coverage"
			LDFLAGS="$LDFLAGS --coverage"
		fi
	])

AC_CONFIG_FILES([Makefile
	include/Makefile include/xscript/Makefile include/internal/Makefile
	library/Makefile library/details/Makefile standard/Makefile standard/pcre/Makefile
	daemon/Makefile xscript/Makefile python/Makefile lua-block/Makefile mist-block/Makefile
	http-block/Makefile file-block/Makefile local-block/Makefile tests/Makefile])

AC_OUTPUT
