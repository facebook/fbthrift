# Copyright 2014 Facebook, Inc.
#
# SYNOPSIS
#
#   AX_CHECK_FOLLY([action-if-found, [action-if-not-found]])
#
# DESCRIPTION
#
#  Look for the folly library in a few default locations, or
#  in the location specified by --with-folly=<dir>
#
#  Sets:
#    HAVE_FOLLY
#    FOLLY_INCLUDES
#    FOLLY_LDFLAGS
#    FOLLY_LIBS

#serial 1

AC_DEFUN([AX_CHECK_FOLLY], [
    folly_searchdirs="/usr/local /usr"
    want_folly=yes
    AC_ARG_WITH([folly],
        [AS_HELP_STRING([--with-folly=DIR],
            [root of the folly installation directory])],
        [
            case "$withval" in
            "" | y | ye | yes)
                ;;
            n | no) want_folly=no
                ;;
            *) folly_searchdirs="$withval"
                ;;
            esac
        ])

    succeeded=no
    if test "x$want_folly" = "xyes"; then
        FOLLY_LIBS="-lfolly"
        for folly_dir in $folly_searchdirs; do
            AC_MSG_CHECKING([for folly/folly-config.h in $folly_dir])
            if test -f "$folly_dir/include/folly/folly-config.h"; then
                FOLLY_INCLUDES="-I$folly_dir/include"
                FOLLY_LDFLAGS="-L$folly_dir/lib"
                AC_MSG_RESULT([yes])
                break
            else
                AC_MSG_RESULT([no])
            fi
        done

        # Even if we couldn't find the header files, try compiling to see if
        # it is already available in our default include/library paths.
        AC_MSG_CHECKING([whether compiling and linking against folly works])

        AC_LANG_PUSH([C++])
        SAVED_LIBS="$LIBS"
        SAVED_LDFLAGS="$LDFLAGS"
        SAVED_CPPFLAGS="$CPPFLAGS"
        LDFLAGS="$LDFLAGS $FOLLY_LDFLAGS"
        LIBS="$FOLLY_LIBS $LIBS"
        CPPFLAGS="$FOLLY_INCLUDES $CPPFLAGS"

        AC_LINK_IFELSE(
            [AC_LANG_PROGRAM([[
                    #include <folly/String.h>
                    #include <cerrno>
                ]], [[
                    folly::errnoStr(ENOENT);
                    return 0;
                ]])],
            [
                AC_MSG_RESULT([yes])
                succeeded=yes
            ], [
                AC_MSG_RESULT([no])
            ])

        CPPFLAGS="$SAVED_CPPFLAGS"
        LDFLAGS="$SAVED_LDFLAGS"
        LIBS="$SAVED_LIBS"
        AC_LANG_POP([C++])
    fi

    if test "$succeeded" != "yes" ; then
        # Not found.  Execute action-if-not-found
        AC_MSG_NOTICE([Unable to find the folly library.])
        ifelse([$2], , :, [$2])
    else
        AC_SUBST([FOLLY_INCLUDES])
        AC_SUBST([FOLLY_LIBS])
        AC_SUBST([FOLLY_LDFLAGS])
        AC_DEFINE(HAVE_FOLLY,,[define if the folly library is available])
        # Execute action-if-found
        ifelse([$1], , :, [$1])
    fi
])
