## Process this file with automake to produce a Makefile.in.

DOCBOOK2TEXI = docbook2texi

info_TEXINFOS = vx68k.texi

MAINTAINERCLEANFILES = vx68k.texi

EXTRA_DIST = vx68k.xml

${srcdir}/vx68k.texi: vx68k.xml
	${DOCBOOK2TEXI} ${srcdir}/vx68k.xml > vx68k.tmp1
	echo '@setfilename vx68k.info' > vx68k.tmp2
	sed -e '/^@setfilename /d' -e '1r vx68k.tmp2' vx68k.tmp1 > ${srcdir}/vx68k.texi
	rm -f vx68k.tmp*
