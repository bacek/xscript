AC_DEFUN([AX_BOOST_SYSTEM],
[
        ax_boost_system_stored_ldflags="$LDFLAGS"
        
        AC_REQUIRE([AX_BOOST_PREFIX])
        AX_BOOST_LIB([ax_boost_system_lib], [boost_system])
       
        LDFLAGS="$ax_boost_system_stored_ldflags $BOOST_LDFLAGS -l$ax_boost_system_lib"
        
        ax_have_boost_system="yes"
	AX_BOOST_HEADER([system/config.hpp], [], [ax_have_boost_system="no"])
        
        LDFLAGS="$ax_boost_system_stored_ldflags"
        
        if test "f$ax_have_boost_system" = "fyes"; then
                ifelse([$1], , :, [$1])
                AC_SUBST([BOOST_SYSTEM_LDFLAGS], ["-l$ax_boost_system_lib"])
        else
                ifelse([$2], , :, [$2])
        fi      
])
