## Process this file with automake to produce a Makefile.in.

EXTRA_DIST = MEMO.ja VERSION-ID

SUBDIRS = include lib bin

dist-snapshot distcheck-snapshot:
	${MAKE} VERSION=${VERSION}.`date +%Y%m%d` `echo $@ | sed -e 's/-snapshot//'`

