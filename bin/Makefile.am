## Process this file with automake to produce a Makefile.in.

INCLUDES = -I${top_srcdir}/include \
${GTK_CFLAGS}

bin_PROGRAMS = vx68k

vx68k_SOURCES = getopt.c getopt1.c main.cc
vx68k_LDADD = ../libvx68kgtk/libvx68kgtk.a \
../libvx68k/libvx68k.a ../libvm68k/libvm68k.la \
${GTK_LIBS}

noinst_HEADERS = getopt.h

man_MANS = vx68k.1

EXTRA_DIST = ${man_MANS}

