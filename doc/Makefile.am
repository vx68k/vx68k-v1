## Process this file with automake to produce a Makefile.in.

DOCBOOK2TEXI = $(srcdir)/docbook2texi

info_TEXINFOS = vx68k.texi

MAINTAINERCLEANFILES = vx68k.texi

EXTRA_DIST = vx68k.xml docbook2texi

${srcdir}/vx68k.texi: vx68k.xml
	${DOCBOOK2TEXI} ${srcdir}/vx68k.xml > $@.tmp
	mv $@.tmp $@
