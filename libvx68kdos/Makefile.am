## Process this file with automake to produce a Makefile.in.

INCLUDES = -I$(top_srcdir)/include -I$(prefix)/include

LIBS =

lib_LTLIBRARIES = libvx68kdos.la

libvx68kdos_la_LDFLAGS = $(LTLIBRELEASE) -version-info 1:1:0
libvx68kdos_la_SOURCES = dos.cc shell.cc doscontext.cc \
filesystem.cc allocator.cc
