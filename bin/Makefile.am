## Process this file with automake to produce a Makefile.in.

INCLUDES = -I$(top_srcdir)/include -I$(prefix)/include \
-DLOCALEDIR=\"$(datadir)/locale\" \
$(GTK_CFLAGS)

noinst_LIBRARIES = libmisc.a
bin_PROGRAMS = vx68k

libmisc_a_SOURCES = getopt.c getopt1.c

vx68k_SOURCES = main.cc gtkconwin.cc gtkabout.cc
vx68k_LDADD = libmisc.a ../vx68kui/libvx68kui_gtk.a \
../libvx68kdos/libvx68kdos.la ../libvx68k/libvx68k.la \
-L$(prefix)/lib -lvm68k $(GTK_LIBS)

noinst_HEADERS = getopt.h gtkapp.h

man_MANS = vx68k.1

EXTRA_DIST = ${man_MANS}
