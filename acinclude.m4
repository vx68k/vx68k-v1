AC_DEFUN(HS_CPLUSPLUS_EXCEPTIONS,
[AC_CACHE_CHECK([for exceptions], hs_cv_cplusplus_exceptions,
[AC_LANG_SAVE[]dnl
AC_LANG_CPLUSPLUS[]dnl
AC_TRY_COMPILE([#include <exception>],
[try {} catch (std::exception &e) {throw;}],
hs_cv_cplusplus_exceptions=yes, hs_cv_cplusplus_exceptions=no)
AC_LANG_RESTORE])
if test "$hs_cv_cplusplus_exceptions" = yes; then
  AC_DEFINE(HAVE_EXCEPTIONS)
fi
])
