## -*-Makefile-*-
## Process this file with automake to produce a Makefile.in.

INCLUDES = -I${top_srcdir}/include

noinst_LIBRARIES = libvx68k.a
bin_PROGRAMS = vx68k

libvx68k_a_SOURCES = memory.cc dos.cc

vx68k_SOURCES = main.cc
vx68k_LDADD = libvx68k.a ../vm68k/libvm68k.a

