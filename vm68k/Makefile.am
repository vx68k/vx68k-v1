## Process this file with automake to produce a Makefile.in.

INCLUDES = -I${top_srcdir}/include

lib_LTLIBRARIES = libvm68k.la

# XXX remove -release for public release.
libvm68k_la_LDFLAGS = -release 1.1snap -version-info 0:0:0
libvm68k_la_SOURCES = memory.cc addrspace.cc \
statusreg.cc execunit.cc context.cc

