## Process this file with automake to produce a Makefile.in.

INCLUDES = -I${top_srcdir}/include

LIBS =

lib_LTLIBRARIES = libvx68k.la

# XXX use the latter for releases.
libvx68k_la_LDFLAGS = -release ${VERSION} -version-info 0
#libvx68k_la_LDFLAGS = -version-info 0:0:0

libvx68k_la_SOURCES = version.cc mainmem.cc gvideomem.cc textvram.cc \
crtcmem.cc palettemem.cc areaset.cc mfpmem.cc sysportmem.cc \
opmmem.cc sccmem.cc ppimem.cc \
spritemem.cc sram.cc fontrom.cc \
iocsdisk.cc systemrom.cc machine.cc x68kaddr.cc \
allocator.cc filesystem.cc doscontext.cc shell.cc dos.cc

CLEANFILES = *.gdb

libvx68k.gdb: $(libvx68k_la_SOURCES)
	nana $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(libvx68k_la_SOURCES) > $@
