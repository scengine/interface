dnl SCE_CHECK_PTHREAD
dnl -----------------
dnl Checks for PThread and fails if not found
AC_DEFUN([SCE_CHECK_PTHREAD],
[
    SCE_REQUIRE_HEADER([pthread.h])
    SCE_REQUIRE_LIB([pthread], [pthread_create])
    dnl those are fake flags in case they will become useful, but currently
    dnl AC_CHECK_LIB (called through SCE_REQUIRE_LIB) fills LIBS as needed
    AC_SUBST([PTHREAD_CFLAGS], [])
    AC_SUBST([PTHREAD_LIBS], [])
])
