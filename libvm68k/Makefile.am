## Process this file with automake to produce a Makefile.in.

INCLUDES = -I${top_srcdir}/include -DVM68K_ENABLE_DEPRECATED

LIBS =

lib_LTLIBRARIES = libvm68k.la

# XXX use the latter for releases.
libvm68k_la_LDFLAGS = -release ${VERSION} -version-info 0
#libvm68k_la_LDFLAGS = -version-info 1:0:0
libvm68k_la_SOURCES = memory.cc addrspace.cc \
statusreg.cc execunit.cc context.cc

