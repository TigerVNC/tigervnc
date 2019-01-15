#!/bin/bash

set -e
set -x

## Basic variables

CURDIR=$(dirname $(readlink -f $0))
TOPDIR=$(git rev-parse --show-toplevel 2>/dev/null)

DEBDIR=${TOPDIR}/contrib/packages/deb/ubuntu-xenial

VERSION=$(grep '^set(VERSION ' ${TOPDIR}/CMakeLists.txt | sed 's@^set(VERSION \(.*\))@\1@')

## Prepare the build directory

rm -rf ${CURDIR}/build
mkdir -p ${CURDIR}/build
chmod a+w ${CURDIR}/build
[ -x /usr/sbin/selinuxenabled ] && /usr/sbin/selinuxenabled && chcon -Rt container_file_t ${CURDIR}/build

## Copy over the source code

(cd ${TOPDIR} && git archive --prefix tigervnc-${VERSION}/ HEAD) | xz > ${CURDIR}/build/tigervnc_${VERSION}.orig.tar.xz

## Copy over the packaging files

cp -r ${DEBDIR}/debian ${CURDIR}/build/debian

chmod a+x ${CURDIR}/build/debian/rules

# Assemble a fake changelog entry to get the correct version

cat - > ${CURDIR}/build/debian/changelog << EOT
tigervnc (${VERSION}-1ubuntu1) UNRELEASED; urgency=low

  * Automated build for TigerVNC

 -- Build bot <tigervncbot@tigervnc.org>  $(date -R)

EOT
cat ${DEBDIR}/debian/changelog >> ${CURDIR}/build/debian/changelog

## Start the build

docker run --volume ${CURDIR}/build:/home/deb/build --interactive --tty --rm tigervnc/${DOCKER} \
	bash -c "
	tar -C ~/build -axf ~/build/tigervnc_${VERSION}.orig.tar.xz &&
	cp -a ~/build/debian ~/build/tigervnc-${VERSION}/debian &&
	mk-build-deps ~/build/tigervnc-${VERSION}/debian/control &&
	sudo dpkg --unpack ~/tigervnc-build-deps_*.deb &&
	sudo apt-get -f install -y &&
	cd ~/build/tigervnc-${VERSION} && dpkg-buildpackage
	"
