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
curl -L -o xorg-server-1.11.4-0ubuntu10.3.tar.gz 'http://anonscm.debian.org/gitweb/?p=pkg-xorg/xserver/xorg-server.git;a=snapshot;h=cbf435a091906484112f5c4cf35b17738e779ce9;sf=tgz'
tar xzf xorg-server-*.tar.gz
rm xorg-server-*.tar.gz
pushd xorg-server-*
QUILT_PATCHES=debian/patches quilt push -a
popd
cp -r xorg-server-*/* unix/xserver 
rm -rf xorg-server-*
pushd unix/xserver
for all in `find . -type f -perm -001`; do
        chmod -x "$all"
done
patch -p1 -b --suffix .vnc < ../xserver111.patch
popd
popd
if [ -e tigervnc_1.3.80.orig.tar.gz ] ; then
	rm tigervnc_1.3.80.orig.tar.gz
fi
tar czf tigervnc_1.3.80.orig.tar.gz tigervnc-1.3.80
rm -rf tigervnc-1.3.80
