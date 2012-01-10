dnl SCE_REQUIRE_HEADER(HEADER, [IF-PRESENT])
dnl --------------------------------------
dnl Check presence of HEADER and exits if not present, or executes IF-PRESENT if
dnl present
AC_DEFUN([SCE_REQUIRE_HEADER],
[
    AC_CHECK_HEADERS([$1],
                     [$2],
                     [AC_MSG_ERROR([$1 not found])])
])

dnl SCE_REQUIRE_LIB(LIBRARY, FUNC, [IF-PRESENT])
dnl ------------------------------------------
dnl Check presence of HEADER and exits if not present, or executes IF-PRESENT if
dnl present
AC_DEFUN([SCE_REQUIRE_LIB],
[
    AC_CHECK_LIB([$1], [$2],
                 [$3],
                 [AC_MSG_ERROR([$1 not found])])
])

dnl SCE_REQUIRE_FUNC(FUNC, [ACTION-IF-FOUND])
dnl -----------------------------------------
dnl Like AC_CHECK_FUNC but exits with an error if FUNC cannot be found
AC_DEFUN([SCE_REQUIRE_FUNC],
[
    AC_CHECK_FUNC([$1],
                  [$2],
                  [AC_MSG_ERROR([$1 not found])])
])

dnl SCE_REQUIRE_FUNCS(FUNCS, [ACTIO-IF-FOUND])
dnl ------------------------------------------
dnl Like AC_CHECK_FUNCS but exits with an error if one of FUCS cannot be found
AC_DEFUN([SCE_REQUIRE_FUNCS],
[
    for func in $1; do
        SCE_REQUIRE_FUNC([$func], [$2])
    done
])
