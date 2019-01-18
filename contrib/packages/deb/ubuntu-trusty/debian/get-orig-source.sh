#!/bin/bash

#curl -L -o tigervnc-1.3.0.tar.bz2 "http://downloads.sourceforge.net/project/tigervnc/tigervnc/1.3.0/tigervnc-1.3.0.tar.bz2"
#tar xjf tigervnc-*.tar.bz2
#rm tigervnc-*.tar.bz2
curl -OL http://sourceforge.net/code-snapshots/svn/t/ti/tigervnc/code/tigervnc-code-5136-trunk.zip
unzip tigervnc-code-*-trunk.zip
mv tigervnc-code-*-trunk tigervnc-1.3.80
rm tigervnc-code-*-trunk.zip
pushd tigervnc-*
curl -L -o fltk-1.3.2-source.tar.gz 'http://anonscm.debian.org/gitweb/?p=users/ucko/fltk1.3.git;a=snapshot;h=HEAD;sf=tgz'
tar xzf fltk-*-source.tar.gz
rm fltk-*-source.tar.gz
mv fltk1.3-* fltk-1.3.2
pushd fltk-*
sh ../../debian/patch_fltk.sh
find . -name "*.orig" -exec rm {} \;
popd
popd
if [ -e tigervnc_1.3.80.orig.tar.gz ] ; then
	rm tigervnc_1.3.80.orig.tar.gz
fi
tar czf tigervnc_1.3.80.orig.tar.gz tigervnc-1.3.80
rm -rf tigervnc-1.3.80
