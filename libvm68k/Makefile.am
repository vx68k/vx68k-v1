## Process this file with automake to produce a Makefile.in.

INCLUDES = -I${top_srcdir}/include

LIBS =

lib_LTLIBRARIES = libvm68k.la

# XXX use the latter for releases.
libvm68k_la_LDFLAGS = -release ${VERSION} -version-info 0
#libvm68k_la_LDFLAGS = -version-info 1:0:0
libvm68k_la_SOURCES = execunit.cc inst0.cc inst1to3.cc inst4.cc inst5.cc \
inst6.cc inst7.cc inst8.cc inst9.cc inst11.cc inst12.cc inst13.cc inst14.cc \
context.cc statusreg.cc memory_map.cc defaultmem.cc memory.cc

noinst_HEADERS = inst.h

CLEANFILES = *.gdb

SUFFIXES = .gdb

.cc.gdb:
	nana $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $<  > $@
