## Process this file with automake to produce a Makefile.in.

INCLUDES = -I${top_srcdir}/include \
${GTK_CFLAGS}

noinst_LIBRARIES = libvx68k.a libvx68kgtk.a

libvx68k_a_SOURCES = mainmem.cc textvram.cc machine.cc \
allocator.cc filesystem.cc doscontext.cc shell.cc dos.cc

libvx68kgtk_a_SOURCES = gtkconsole.cc

