## Process this file with automake to produce a Makefile.in.

INCLUDES = -I${top_srcdir}/include

bin_PROGRAMS = x68k

x68k_SOURCES = main.cc
x68k_LDADD = vx68k/libvx68k.a vm68k/libvm68k.a

SUBDIRS = include vm68k vx68k

dist-snapshot:
	${MAKE} VERSION=${VERSION}x.`date +%Y%m%d` dist

