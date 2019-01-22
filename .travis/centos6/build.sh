#!/bin/bash

set -e
set -x

## Basic variables

CURDIR=$(dirname $(readlink -f $0))
TOPDIR=$(git rev-parse --show-toplevel 2>/dev/null)

RPMDIR=${TOPDIR}/contrib/packages/rpm/el6

VERSION=$(grep '^set(VERSION ' ${TOPDIR}/CMakeLists.txt | sed 's@^set(VERSION \(.*\))@\1@')

## Prepare the build directory

rm -rf ${CURDIR}/rpmbuild
mkdir -p ${CURDIR}/rpmbuild/{BUILD,BUILDROOT,SRPMS,SOURCES,SPECS,RPMS}
chmod a+w ${CURDIR}/rpmbuild/{BUILD,BUILDROOT,SRPMS,RPMS}
[ -x /usr/sbin/selinuxenabled ] && /usr/sbin/selinuxenabled && chcon -Rt container_file_t ${CURDIR}/rpmbuild

## Copy over the packaging files

cp ${RPMDIR}/SOURCES/* ${CURDIR}/rpmbuild/SOURCES
cp ${RPMDIR}/SPECS/tigervnc.spec ${CURDIR}/rpmbuild/SPECS
sed -i "s/@VERSION@/${VERSION}/" ${CURDIR}/rpmbuild/SPECS/tigervnc.spec

# Extra dependencies built because the distribution lacks what we need

curl -L -o ${CURDIR}/rpmbuild/SOURCES/fltk-1.3.4-2-source.tar.gz https://www.fltk.org/pub/fltk/1.3.4/fltk-1.3.4-2-source.tar.gz
curl -L -o ${CURDIR}/rpmbuild/SOURCES/libpng-1.6.34.tar.gz https://downloads.sourceforge.net/project/libpng/libpng16/1.6.34/libpng-1.6.34.tar.gz
curl -L -o ${CURDIR}/rpmbuild/SOURCES/gmp-6.1.2.tar.bz2 https://ftp.gnu.org/gnu/gmp/gmp-6.1.2.tar.bz2
curl -L -o ${CURDIR}/rpmbuild/SOURCES/libtasn1-4.13.tar.gz https://ftp.gnu.org/gnu/libtasn1/libtasn1-4.13.tar.gz
curl -L -o ${CURDIR}/rpmbuild/SOURCES/nettle-3.4.tar.gz https://ftp.gnu.org/gnu/nettle/nettle-3.4.tar.gz
curl -L -o ${CURDIR}/rpmbuild/SOURCES/gnutls-3.3.30.tar.xz https://www.gnupg.org/ftp/gcrypt/gnutls/v3.3/gnutls-3.3.30.tar.xz

## Copy over the source code

(cd ${TOPDIR} && git archive --prefix tigervnc-${VERSION}/ HEAD) | bzip2 > ${CURDIR}/rpmbuild/SOURCES/tigervnc-${VERSION}.tar.bz2

## Start the build

docker run --volume ${CURDIR}/rpmbuild:/home/rpm/rpmbuild --interactive --tty --rm tigervnc/${DOCKER} \
	bash -c "
	sudo yum-builddep -y ~/rpmbuild/SPECS/tigervnc.spec &&
        sudo chown 0.0 ~/rpmbuild/SOURCES/* &&
        sudo chown 0.0 ~/rpmbuild/SPECS/* &&
	rpmbuild -ba ~/rpmbuild/SPECS/tigervnc.spec
	"
