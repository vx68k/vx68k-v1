## Process this file with automake to produce a Makefile.in.

INCLUDES = -I$(top_srcdir)/include -I$(prefix)/include

LIBS =

lib_LTLIBRARIES = libvx68k.la libvx68kdos.la

# XXX remove the flag `-release' for stable releases.
libvx68k_la_LDFLAGS = -version-info 0:0:0 -release ${VERSION}
libvx68k_la_SOURCES = version.cc x68kaddr.cc machine.cc \
mainmem.cc gvideomem.cc textvram.cc \
crtcmem.cc palettemem.cc dmacmem.cc areaset.cc mfpmem.cc sysportmem.cc \
opmmem.cc msm6258vmem.cc fdcmem.cc sccmem.cc ppimem.cc \
spritemem.cc sram.cc fontrom.cc \
iocsdisk.cc systemrom.cc

# XXX remove the flag `-release' for stable releases.
libvx68kdos_la_LDFLAGS = -version-info 0:0:0 -release ${VERSION}
libvx68kdos_la_SOURCES = dos.cc shell.cc doscontext.cc \
filesystem.cc allocator.cc

CLEANFILES = *.gdb

SUFFIXES = .gdb

.cc.gdb:
	nana $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $<  > $@
