## Process this file with automake to produce a Makefile.in.

INCLUDES = -I${top_srcdir}/include

bin_PROGRAMS = vx68k

vx68k_SOURCES = main.cc
vx68k_LDADD = lib/vx68k/libvx68k.a lib/vm68k/libvm68k.a

EXTRA_DIST = VERSION-ID

SUBDIRS = include lib

dist-snapshot distcheck-snapshot:
	${MAKE} VERSION=${VERSION}x`date +%Y%m%d` `echo $@ | sed -e 's/-snapshot//'`

