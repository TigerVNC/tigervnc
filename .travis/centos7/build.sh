#!/bin/bash

set -e
set -x

## Basic variables

CURDIR=$(dirname $(readlink -f $0))
TOPDIR=$(git rev-parse --show-toplevel 2>/dev/null)

RPMDIR=${TOPDIR}/contrib/packages/rpm/el7

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
