## Process this file with automake to produce a Makefile.in.

DOCBOOK2TEXI = docbook2texi

info_TEXINFOS = vx68k.texi

MAINTAINERCLEANFILES = vx68k.texi

EXTRA_DIST = vx68k.xml

${srcdir}/vx68k.texi: vx68k.xml
	${DOCBOOK2TEXI} ${srcdir}/vx68k.xml > $@
	if grep '^@setfilename' $@; then :; else \
	    head -n 1 $@ > $@.tmp && \
	    echo '@setfilename vx68k.info' >> $@.tmp && \
	    echo '@settitle Virtual X68000' >> $@.tmp && \
	    tail -n +2 $@ >> $@.tmp && \
	    mv $@.tmp $@; \
	fi
