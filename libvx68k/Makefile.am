## Process this file with automake to produce a Makefile.in.

INCLUDES = -I$(top_srcdir)/include -I$(prefix)/include

LIBS =

lib_LTLIBRARIES = libvx68k.la

libvx68k_la_LDFLAGS = $(LTLIBRELEASE) -version-info 1:1:0
libvx68k_la_SOURCES = version.cc x68kaddr.cc machine.cc \
mainmem.cc gvideomem.cc textvram.cc \
crtcmem.cc palettemem.cc dmacmem.cc areaset.cc mfpmem.cc sysportmem.cc \
opmmem.cc msm6258vmem.cc fdcmem.cc sccmem.cc ppimem.cc \
spritemem.cc sram.cc fontrom.cc \
iocsdisk.cc systemrom.cc
