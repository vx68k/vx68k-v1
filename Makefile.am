## Process this file with automake to produce a Makefile.in.

INCLUDES = -I${top_srcdir}/include

bin_PROGRAMS = vx68k

vx68k_SOURCES = main.cc
vx68k_LDADD = libvx68k/libvx68k.a libvm68k/libvm68k.a

SUBDIRS = include libvm68k libvx68k

dist-snapshot:
	${MAKE} VERSION=${VERSION}x.`date +%Y%m%d` dist

