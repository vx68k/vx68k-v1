## Process this file with automake to produce a Makefile.in.

INCLUDES = -I${top_srcdir}/include \
${GTK_CFLAGS}

bin_PROGRAMS = vx68k

vx68k_SOURCES = getopt.c getopt1.c main.cc
vx68k_LDADD = ../lib/vx68k/libvx68kgtk.a ../lib/vx68k/libvx68k.a \
../vm68k/libvm68k.a \
${GTK_LIBS}

noinst_HEADERS = getopt.h

man_MANS = vx68k.1

EXTRA_DIST = ${man_MANS}

