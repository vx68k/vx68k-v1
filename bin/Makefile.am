## Process this file with automake to produce a Makefile.in.

INCLUDES = -I${top_srcdir}/include

bin_PROGRAMS = vx68k

vx68k_SOURCES = getopt.c getopt1.c main.cc
vx68k_LDADD = ../lib/vx68k/libvx68k.a ../lib/vm68k/libvm68k.a

noinst_HEADERS = getopt.h

