#!/bin/bash

# automatically build debian package from current git commit
#
# example:
#
# contrib/packages/deb/build-from-git.sh contrib/packages/deb/ubuntu-xenial/ /tmp/

set -e
set -o pipefail

usage () {
	echo "usage: $0 LOCATION BUILDDIR"
	exit 1
}

LOCATION="$1"
test -d "$LOCATION" || usage
DEBIAN="$LOCATION"/debian
test -d "$DEBIAN" || { echo "no 'debian' directory found in '$LOCATION'"; usage; }
BUILDDIR="$2"
test -d "$BUILDDIR" || { echo "'$BUILDDIR' is not a directory"; usage; }

cd "$LOCATION"

VERSION="$(dpkg-parsechangelog | awk -F ': ' '/^Version: / {print $2}' | cut -f1 -d-)$(git log -n 1 --pretty=format:'+git.%cd.%h' --date=format:"%Y.%m.%d")"
TARBALL="tigervnc_${VERSION}.orig.tar"
DEBBALL="tigervnc_${VERSION}_build-from-git.debian.tar"
WORKDIR="tigervnc-${VERSION}"

COMMIT="$(git log -n 1 --pretty=format:'%H')"
git archive $COMMIT --output "$BUILDDIR/$DEBBALL" debian

cd "$(git rev-parse --show-toplevel)"
git archive $COMMIT --output "$BUILDDIR/$TARBALL" --prefix "$WORKDIR"/

cd "$BUILDDIR"
tar axf "$TARBALL"
gzip "$TARBALL"
cd "$WORKDIR"
tar axf ../"$DEBBALL"
debchange --newversion "$VERSION"-1 "generated using build-from-git.sh"
dpkg-buildpackage -us -uc
