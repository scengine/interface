dnl SCE_CHECK_DEBUG
dnl ---------------
dnl Checks for various user-chosen options and define SCE_DEBUG_CFLAGS
AC_DEFUN([SCE_CHECK_DEBUG],
[
    AC_ARG_ENABLE([debug_all],
                  AS_HELP_STRING([--enable-debug-all],
                                 [enable all paranoiac and commonly useless debugging @<:@default=no@:>@]),
                  [enable_debug_all="$enableval"
                   AS_IF([test "x$enable_debug_all" = "xyes"],
                         [enable_optimization="no"
                          enable_debug_stack="yes"
                          enable_debug="yes"])],
                  [enable_debug_all="no"])
    AC_ARG_ENABLE([optimization],
                  AS_HELP_STRING([--disable-optimisation],
                                 [disable any compiler optimisation @<:@default=no@:>@]),
                  [enable_optimization="$enableval"],
                  [enable_optimization="yes"])
    AC_ARG_ENABLE([debug_stack],
                  AS_HELP_STRING([--enable-debug-stack],
                                 [enable stack debugging @<:@default=no@:>@]),
                  [enable_debug_stack="$enableval"],
                  [enable_debug_stack="no"])
    AC_ARG_ENABLE([debug],
                  AS_HELP_STRING([--enable-debug],
                                 [enable debugging @<:@default=yes@:>@]),
                  [enable_debug="$enableval"],
                  [enable_debug="yes"])

    SCE_DEBUG_CFLAGS=
    SCE_DEBUG_CFLAGS_EXPORT=

    dnl normal debugging
    AC_MSG_CHECKING([whether to enable debugging])
    AS_IF([test "x$enable_debug" = "xyes"],
          [AC_DEFINE([SCE_DEBUG], [1], [is debugging enabled])
           SCE_DEBUG_CFLAGS_EXPORT="$SCE_DEBUG_CFLAGS_EXPORT -DSCE_DEBUG"
           SCE_DEBUG_CFLAGS="$SCE_DEBUG_CFLAGS -g"
           AC_MSG_RESULT([yes])],
          [DEBUG_CFLAGS=
           AC_MSG_RESULT([no])])
    AC_SUBST([DEBUG_CFLAGS])

    dnl stack debugging
    AC_MSG_CHECKING([whether to enable stack debugging])
    AS_IF([test "x$enable_debug_stack" = "xyes"],
          [SCE_DEBUG_CFLAGS="$SCE_DEBUG_CFLAGS -fstack-protector-all"
           AC_MSG_RESULT([yes])],
          [AC_MSG_RESULT([no])])

    dnl optimization debugging (must be the last change to CFLAGS)
    AC_MSG_CHECKING([whether to disable any compiler optimization])
    AS_IF([test "x$enable_optimization" = "xno"],
          [SCE_DEBUG_CFLAGS="$SCE_DEBUG_CFLAGS -O0"
           AC_MSG_RESULT([yes])],
          [AC_MSG_RESULT([no])])

    AC_SUBST([SCE_DEBUG_CFLAGS])
    AC_SUBST([SCE_DEBUG_CFLAGS_EXPORT])
])
