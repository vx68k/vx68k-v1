## Process this file with automake to produce a Makefile.in.

INCLUDES = -DLOCALEDIR=\"$(datadir)/locale\" \
-I$(top_srcdir)/libvx68k-gtk -I$(top_srcdir)/../include \
$(GTK_CFLAGS)

bin_PROGRAMS = vx68k

vx68k_SOURCES = main.cc gtkconwin.cc gtkabout.cc getopt.c getopt1.c
vx68k_LDADD = ../libvx68k-gtk/libvx68kui_gtk.a \
../../libvx68kdos/libvx68kdos.la ../../libvx68k/libvx68k.la \
$(LIBVM68K) $(GTK_LIBS)

noinst_HEADERS = getopt.h gtkapp.h

man_MANS = vx68k.1

EXTRA_DIST = ${man_MANS}
