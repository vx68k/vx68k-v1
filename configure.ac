dnl Process this file with autoconf to produce a configure script.
AC_INIT([Virtual X68000], 1.1.10, [vx68k@lists.hypercore.co.jp], vx68k)
AC_CONFIG_SRCDIR(libvx68k/machine.cc)
dnl AC_CONFIG_HEADERS(config.h)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(AC_PACKAGE_TARNAME, AC_PACKAGE_VERSION)
LTLIBRELEASE="-release 1.1"
AC_SUBST(LTLIBRELEASE)dnl
AC_DISABLE_SHARED
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_LIBTOOL
AM_PATH_GTK(1.2.0,,, gthread)
AC_SEARCH_LIBS(glGetError, GL)
AC_SEARCH_LIBS(gluErrorString, GLU)
AC_CHECK_HEADERS(fcntl.h unistd.h)
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_CXX_EXCEPTIONS
AC_CXX_TEMPLATES
AC_CXX_NAMESPACES
AC_CHECK_FUNCS(localtime_r)
ALL_LINGUAS="ja"
AM_GNU_GETTEXT
AC_CONFIG_FILES(Makefile intl/Makefile
libvx68kdos/Makefile libvx68kdos/vx68k/Makefile
include/Makefile include/vx68k/Makefile libvx68k/Makefile
libvx68k-gtk/Makefile libvx68k-gtk/vx68k/Makefile
programs/Makefile po/Makefile.in doc/Makefile testsuite/Makefile)
dnl AC_CONFIG_FILES(stamp-h, [echo timestamp > stamp-h])
AC_CONFIG_SUBDIRS(libvm68k)
AC_OUTPUT
if test "$ac_cv_cxx_exceptions" != yes; then
  AC_MSG_WARN([the C++ compiler does not support exceptions])
  AC_MSG_WARN([this program cannot be built without exceptions])
fi

if test "$ac_cv_cxx_templates" != yes; then
  AC_MSG_WARN([the C++ compiler does not support templates])
  AC_MSG_WARN([this program cannot be built without templates])
fi

if test "$ac_cv_cxx_namespaces" != yes; then
  AC_MSG_WARN([the C++ compiler does not implement namespaces])
  AC_MSG_WARN([this program cannot be built without namespaces])
fi
