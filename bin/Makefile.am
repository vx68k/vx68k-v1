## Process this file with automake to produce a Makefile.in.

INCLUDES = -I${top_srcdir}/include \
-DLOCALEDIR=\"$(datadir)/locale\" \
${GTK_CFLAGS}

bin_PROGRAMS = vx68k
noinst_LIBRARIES = libmisc.a

libmisc_a_SOURCES = getopt.c getopt1.c

vx68k_SOURCES = main.cc gtkconwin.cc gtkabout.cc
vx68k_LDADD = libmisc.a ../libvx68kgtk/libvx68kgtk.a \
../libvx68k/libvx68k.la ../libvm68k/libvm68k.la \
${GTK_LIBS}

noinst_HEADERS = getopt.h gtkapp.h

man_MANS = vx68k.1

EXTRA_DIST = ${man_MANS}

