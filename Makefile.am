## Process this file with automake to produce a Makefile.in.

EXTRA_DIST = VERSION-ID

SUBDIRS = include lib bin testsuite

dist-snapshot distcheck-snapshot:
	${MAKE} VERSION=${VERSION}.`date +%Y%m%d` `echo $@ | sed -e 's/-snapshot//'`

