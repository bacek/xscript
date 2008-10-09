AC_DEFUN([AX_CHECK_CPPUNIT],
[
	AC_REQUIRE([AM_PATH_CPPUNIT])
	
	AC_LANG_SAVE
	AC_LANG_CPLUSPLUS
	
	ac_have_cppunit="no"
	ac_cppunit_found="no"
	AC_ARG_ENABLE(cppunit, 
		AS_HELP_STRING(--enable-cppunit,enables cppunit tests compilation),
	[
		if test "f$enableval" = "fyes"; then
			ac_have_cppunit="yes"
		fi
	])

	if test "f$ac_have_cppunit" = "fyes"; then
		ac_cppunit_found="yes"
		AM_PATH_CPPUNIT([1.10.0], [], [ac_cppunit_found="no"])
	
		ac_stored_libs=$LIBS
		ac_stored_cppflags=$CPPFLAGS
		LIBS="$ac_stored_libs $CPPUNIT_LIBS"
		CPPFLAGS="$ac_stored_cppflags $CPPUNIT_CFLAGS"

		AC_MSG_CHECKING([trying to link with cppunit])
		AC_LINK_IFELSE(
			[AC_LANG_PROGRAM([#include <cppunit/ui/text/TestRunner.h>], [CppUnit::TextUi::TestRunner r;])],
			[AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no]); ac_cppunit_found="no"])
	
		CPPFLAGS=$ac_stored_cppflags
		LIBS=$ac_stored_libs
	fi
	if test "f$ac_cppunit_found" != "fyes"; then
		AC_MSG_RESULT([cppunit not found. compilation of tests disabled])
	fi	
	AC_LANG_RESTORE
	AM_CONDITIONAL(HAVE_TESTS, [test "f$ac_cppunit_found" = "fyes"])
])

