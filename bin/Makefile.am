## Process this file with automake to produce a Makefile.in.

INCLUDES = -I${top_srcdir}/include \
${GTK_CFLAGS}

noinst_LIBRARIES = libmisc.a
bin_PROGRAMS = vx68k

libmisc_a_SOURCES = getopt.c getopt1.c

vx68k_SOURCES = main.cc
vx68k_LDADD = libmisc.a ../libvx68kgtk/libvx68kgtk.a \
../libvx68k/libvx68k.la ../libvm68k/libvm68k.la \
${GTK_LIBS}

noinst_HEADERS = getopt.h

man_MANS = vx68k.1

EXTRA_DIST = ${man_MANS}

