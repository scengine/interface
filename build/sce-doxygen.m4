dnl SCE_CHECK_DOXYGEN
dnl -----------------
dnl Checks for Doxygen and defines SCE_DOC_TARGET according to its presence
AC_DEFUN([SCE_CHECK_DOXYGEN],
[
    AC_CHECK_PROGS([DOXYGEN], [doxygen], [:])
    AS_IF([test "x$DOXYGEN" = "x:"],
          [AC_MSG_WARN([Doxygen not found, you will not able to generate documentation])
           SCE_DOC_TARGET='@echo "no compatible doc generator found, please install one then re-run configure" >&2; exit 1'],
          [SCE_DOC_TARGET='$(DOXYGEN) Doxyfile'])
    AC_SUBST([SCE_DOC_TARGET])
])
