## Process this file with automake to produce a Makefile.in.

INCLUDES = -I${top_srcdir}/include

LIBS =

lib_LTLIBRARIES = libvm68k.la

# XXX remove the flag `-release' for stable releases.
libvm68k_la_LDFLAGS = -version-info 0:0:0 -release ${VERSION}
libvm68k_la_SOURCES = execunit.cc inst0.cc inst1to3.cc inst4.cc inst5.cc \
inst6.cc inst7.cc inst8.cc inst9.cc inst11.cc inst12.cc inst13.cc inst14.cc \
context.cc condition_code.cc memory_map.cc memory.cc

noinst_HEADERS = instr.h

CLEANFILES = *.gdb

SUFFIXES = .gdb

.cc.gdb:
	nana $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $<  > $@
