%define _default_patch_fuzz 2
%define tigervnc_src_dir %{_builddir}/%{name}-%{version}%{?snap:-%{snap}}
%{!?_self_signed: %define _self_signed 1}
%{!?_bootstrap: %define _bootstrap 1}
#%global scl_name %{name}$(echo %version | sed -e 's/\\.//;s/\\..*//')
%define scl_name %{name}16
%if %{_bootstrap}
%define xorg_buildroot $RPM_BUILD_ROOT/opt/%{name}/%{scl_name}
%else
%define xorg_buildroot /opt/%{name}/%{scl_name}
%endif

Name: tigervnc
Version: @VERSION@
Release: 3%{?snap:.%{snap}}%{?dist}
Summary: A TigerVNC remote display system

Group: User Interface/Desktops
License: GPLv2+
Packager: Brian P. Hinz <bphinz@users.sourceforge.net>
URL: http://www.tigervnc.com

Source0: %{name}-%{version}%{?snap:-%{snap}}.tar.bz2
Source1: vncserver.service
Source2: vncserver.sysconfig
Source9: FindX11.cmake
Source11: http://fltk.org/pub/fltk/1.3.3/fltk-1.3.3-source.tar.gz
Source12: http://downloads.sourceforge.net/project/libjpeg-turbo/1.4.2/libjpeg-turbo-1.4.2.tar.gz
Source13: http://downloads.sourceforge.net/project/libpng/libpng15/1.5.24/libpng-1.5.24.tar.bz2
Source14: https://ftp.gnu.org/gnu/gmp/gmp-6.0.0a.tar.bz2
Source15: http://ftp.gnu.org/gnu/libtasn1/libtasn1-4.7.tar.gz
Source16: https://ftp.gnu.org/gnu/nettle/nettle-2.7.1.tar.gz
Source17: ftp://ftp.gnutls.org/gcrypt/gnutls/v3.3/gnutls-3.3.19.tar.xz

Source100: http://www.x.org/releases/X11R7.7/src/everything/bigreqsproto-1.1.2.tar.bz2
Source101: http://www.x.org/releases/X11R7.7/src/everything/compositeproto-0.4.2.tar.bz2
Source102: http://www.x.org/releases/X11R7.7/src/everything/damageproto-1.2.1.tar.bz2
Source103: http://www.x.org/releases/X11R7.7/src/everything/dmxproto-2.3.1.tar.bz2
Source104: http://www.x.org/releases/X11R7.7/src/everything/dri2proto-2.6.tar.bz2
Source105: http://www.x.org/releases/X11R7.7/src/everything/fixesproto-5.0.tar.bz2
Source106: http://www.x.org/releases/X11R7.7/src/everything/font-util-1.3.0.tar.bz2
Source107: http://www.x.org/releases/X11R7.7/src/everything/fontsproto-2.1.2.tar.bz2
Source108: http://www.x.org/releases/X11R7.7/src/everything/glproto-1.4.15.tar.bz2
Source109: http://www.x.org/releases/X11R7.7/src/everything/inputproto-2.2.tar.bz2
Source110: http://www.x.org/releases/X11R7.7/src/everything/kbproto-1.0.6.tar.bz2
Source111: http://www.x.org/releases/X11R7.7/src/everything/libICE-1.0.8.tar.bz2
Source112: http://www.x.org/releases/X11R7.7/src/everything/libSM-1.2.1.tar.bz2
Source113: http://www.x.org/releases/X11R7.7/src/everything/libX11-1.5.0.tar.bz2
Source114: http://www.x.org/releases/X11R7.7/src/everything/libXScrnSaver-1.2.2.tar.bz2
Source115: http://www.x.org/releases/X11R7.7/src/everything/libXau-1.0.7.tar.bz2
Source116: http://www.x.org/releases/X11R7.7/src/everything/libXaw-1.0.11.tar.bz2
Source117: http://www.x.org/releases/X11R7.7/src/everything/libXcomposite-0.4.3.tar.bz2
Source118: http://www.x.org/releases/X11R7.7/src/everything/libXcursor-1.1.13.tar.bz2
Source119: http://www.x.org/releases/X11R7.7/src/everything/libXdamage-1.1.3.tar.bz2
Source120: http://www.x.org/releases/X11R7.7/src/everything/libXdmcp-1.1.1.tar.bz2
Source121: http://www.x.org/releases/X11R7.7/src/everything/libXext-1.3.1.tar.bz2
Source122: http://www.x.org/releases/X11R7.7/src/everything/libXfixes-5.0.tar.bz2
Source123: http://www.x.org/releases/X11R7.7/src/everything/libXfont-1.4.5.tar.bz2
Source124: http://www.x.org/releases/X11R7.7/src/everything/libXft-2.3.1.tar.bz2
Source125: http://www.x.org/releases/X11R7.7/src/everything/libXi-1.6.1.tar.bz2
Source126: http://www.x.org/releases/X11R7.7/src/everything/libXinerama-1.1.2.tar.bz2
Source127: http://www.x.org/releases/X11R7.7/src/everything/libXmu-1.1.1.tar.bz2
Source128: http://www.x.org/releases/X11R7.7/src/everything/libXpm-3.5.10.tar.bz2
Source129: http://www.x.org/releases/X11R7.7/src/everything/libXrandr-1.3.2.tar.bz2
Source130: http://www.x.org/releases/X11R7.7/src/everything/libXrender-0.9.7.tar.bz2
Source131: http://www.x.org/releases/X11R7.7/src/everything/libXres-1.0.6.tar.bz2
Source132: http://www.x.org/releases/X11R7.7/src/everything/libXt-1.1.3.tar.bz2
Source133: http://www.x.org/releases/X11R7.7/src/everything/libXtst-1.2.1.tar.bz2
Source134: http://www.x.org/releases/X11R7.7/src/everything/libXv-1.0.7.tar.bz2
Source135: http://www.x.org/releases/X11R7.7/src/everything/libXvMC-1.0.7.tar.bz2
Source136: http://www.x.org/releases/X11R7.7/src/everything/libXxf86dga-1.1.3.tar.bz2
Source137: http://www.x.org/releases/X11R7.7/src/everything/libXxf86vm-1.1.2.tar.bz2
Source138: http://www.x.org/releases/X11R7.7/src/everything/libfontenc-1.1.1.tar.bz2
Source139: http://www.x.org/releases/X11R7.7/src/everything/libpciaccess-0.13.1.tar.bz2
#Source140: http://www.x.org/releases/X11R7.7/src/everything/libpthread-stubs-0.3.tar.bz2
# libpthread-stubs fails to compile, so we use the same method
# as the el6 libxcb rpm. pthread-stubs.pc.in taken from el6 libxcb rpm
Source140: pthread-stubs.pc.in
Source141: http://www.x.org/releases/X11R7.7/src/everything/libxcb-1.8.1.tar.bz2
Source142: http://www.x.org/releases/X11R7.7/src/everything/libxkbfile-1.0.8.tar.bz2
Source143: http://www.x.org/releases/X11R7.7/src/everything/makedepend-1.0.4.tar.bz2
Source144: http://www.x.org/releases/X11R7.7/src/everything/randrproto-1.3.2.tar.bz2
Source145: http://www.x.org/releases/X11R7.7/src/everything/recordproto-1.14.2.tar.bz2
Source146: http://www.x.org/releases/X11R7.7/src/everything/renderproto-0.11.1.tar.bz2
Source147: http://www.x.org/releases/X11R7.7/src/everything/resourceproto-1.2.0.tar.bz2
Source148: http://www.x.org/releases/X11R7.7/src/everything/scrnsaverproto-1.2.2.tar.bz2
Source149: http://www.x.org/releases/X11R7.7/src/everything/util-macros-1.17.tar.bz2
Source150: http://www.x.org/releases/X11R7.7/src/everything/videoproto-2.3.1.tar.bz2
Source151: http://www.x.org/releases/X11R7.7/src/everything/xcb-proto-1.7.1.tar.bz2
Source152: http://www.x.org/releases/X11R7.7/src/everything/xcmiscproto-1.2.2.tar.bz2
Source153: http://www.x.org/releases/X11R7.7/src/everything/xextproto-7.2.1.tar.bz2
Source154: http://www.x.org/releases/X11R7.7/src/everything/xf86bigfontproto-1.2.0.tar.bz2
Source155: http://www.x.org/releases/X11R7.7/src/everything/xf86dgaproto-2.1.tar.bz2
Source156: http://www.x.org/releases/X11R7.7/src/everything/xf86driproto-2.1.1.tar.bz2
Source157: http://www.x.org/releases/X11R7.7/src/everything/xf86vidmodeproto-2.3.1.tar.bz2
Source158: http://www.x.org/releases/X11R7.7/src/everything/xineramaproto-1.2.1.tar.bz2
Source159: http://www.x.org/releases/X11R7.7/src/everything/xorg-server-1.12.2.tar.bz2
Source160: http://www.x.org/releases/X11R7.7/src/everything/xproto-7.0.23.tar.bz2
Source161: http://www.x.org/releases/X11R7.7/src/everything/xrandr-1.3.5.tar.bz2
Source162: http://www.x.org/releases/X11R7.7/src/everything/xtrans-1.2.7.tar.bz2

Source200: http://fontconfig.org/release/fontconfig-2.8.0.tar.gz
Source201: http://download.savannah.gnu.org/releases/freetype/freetype-old/freetype-2.3.11.tar.bz2
Source202: http://xorg.freedesktop.org/archive/individual/lib/pixman-0.32.4.tar.bz2
Source203: http://dri.freedesktop.org/libdrm/libdrm-2.4.52.tar.bz2
Source204: ftp://ftp.freedesktop.org/pub/mesa/older-versions/9.x/9.2.5/MesaLib-9.2.5.tar.bz2
# NOTE:
#   libgcrypt from el5 is not new enough to satisfy newer Xorg requirements for --with-sha1,
#   which causes Xorg to link against libssl.so and introduce about 10 dynamic dependencies.
#   to prevent this, build a static libsha1 and link against that.
# NOTE:
Source205: https://github.com/dottedmag/libsha1/archive/0.3.tar.gz

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

# xorg requires newer versions of automake, & autoconf than are available with el5. Use el6 versions.
BuildRequires: automake >= 1.11, autoconf >= 2.60, libtool >= 1.4, gettext >= 0.14.4, gettext-devel >= 0.14.4, bison-devel, python26
BuildRequires: java-devel, jpackage-utils
BuildRequires: pam-devel
BuildRequires: cmake28
BuildRequires: pkgconfig >= 0.20
BuildRequires: gcc44, gcc44-c++
BuildRequires: glibc-devel, libstdc++-devel, libpng-devel
BuildRequires: expat-devel
BuildRequires: git, gperf, intltool, libtalloc-devel
BuildRequires: kernel-headers, libatomic_ops-devel
BuildRequires: xz
%if !%{_bootstrap}
BuildRequires: %{name}-static-devel == %{version}
BuildRequires: nasm >= 2.01
%endif

Requires(post): initscripts chkconfig coreutils
Requires(postun):coreutils
Requires: hicolor-icon-theme
Requires: tigervnc-license

Provides: vnc = 4.1.3-2, vnc-libs = 4.1.3-2
Obsoletes: vnc < 4.1.3-2, vnc-libs < 4.1.3-2
Provides: tightvnc = 1.5.0-0.15.20090204svn3586
Obsoletes: tightvnc < 1.5.0-0.15.20090204svn3586

# tigervnc patches
Patch12: tigervnc14-static-build-fixes.patch
Patch13: tigervnc14-Add-dridir-param.patch
Patch14: tigervnc14-Add-xkbcompdir-param.patch

# fltk patches
Patch15: fltk-1.3.3-static-libs.patch

# freetype patches
Patch20:  freetype-2.1.10-enable-ft2-bci.patch
Patch21:  freetype-2.3.0-enable-spr.patch

# Enable otvalid and gxvalid modules
Patch46:  freetype-2.2.1-enable-valid.patch

# Fix multilib conflicts
Patch88:  freetype-multilib.patch

Patch89:  freetype-2.3.11-CVE-2010-2498.patch
Patch90:  freetype-2.3.11-CVE-2010-2499.patch
Patch91:  freetype-2.3.11-CVE-2010-2500.patch
Patch92:  freetype-2.3.11-CVE-2010-2519.patch
Patch93:  freetype-2.3.11-CVE-2010-2520.patch
Patch96:  freetype-2.3.11-CVE-2010-1797.patch
Patch97:  freetype-2.3.11-CVE-2010-2805.patch
Patch98:  freetype-2.3.11-CVE-2010-2806.patch
Patch99:  freetype-2.3.11-CVE-2010-2808.patch
Patch100:  freetype-2.3.11-CVE-2010-3311.patch
Patch101:  freetype-2.3.11-CVE-2010-3855.patch
Patch102:  freetype-2.3.11-CVE-2011-0226.patch
Patch103:  freetype-2.3.11-CVE-2011-3256.patch
Patch104:  freetype-2.3.11-CVE-2011-3439.patch
Patch105:  freetype-2.3.11-CVE-2012-1126.patch
Patch106:  freetype-2.3.11-CVE-2012-1127.patch
Patch107:  freetype-2.3.11-CVE-2012-1130.patch
Patch108:  freetype-2.3.11-CVE-2012-1131.patch
Patch109:  freetype-2.3.11-CVE-2012-1132.patch
Patch110:  freetype-2.3.11-CVE-2012-1134.patch
Patch111:  freetype-2.3.11-CVE-2012-1136.patch
Patch112:  freetype-2.3.11-CVE-2012-1137.patch
Patch113:  freetype-2.3.11-CVE-2012-1139.patch
Patch114:  freetype-2.3.11-CVE-2012-1140.patch
Patch115:  freetype-2.3.11-CVE-2012-1141.patch
Patch116:  freetype-2.3.11-CVE-2012-1142.patch
Patch117:  freetype-2.3.11-CVE-2012-1143.patch
Patch118:  freetype-2.3.11-CVE-2012-1144.patch
Patch119:  freetype-2.3.11-bdf-overflow.patch
Patch120:  freetype-2.3.11-array-initialization.patch
Patch121:  freetype-2.3.11-CVE-2012-5669.patch

# Patches for Xorg CVE-2014-12-09 taken from Debian:
# https://release.debian.org/proposed-updates/stable_diffs/xorg-server_1.12.4-6+deb7u5.debdiff
Patch10000: 16_CVE-2014-mult.diff
Patch10001: 17_CVE-regressions.diff
# http://www.x.org/wiki/Development/Security/Advisory-2015-02-10/
Patch10002: CVE-2015-0255.diff
# http://www.x.org/wiki/Development/Security/Advisory-2015-03-17/
Patch10003: CVE-2015-1802.diff
Patch10004: CVE-2015-1803.diff
Patch10005: CVE-2015-1804.diff
# http://lists.x.org/archives/xorg-announce/2015-April/002561.html
Patch10006: CVE-2013-7439.diff

%description
Virtual Network Computing (VNC) is a remote display system which
allows you to view a computing 'desktop' environment not only on the
machine where it is running, but from anywhere on the Internet and
from a wide variety of machine architectures.  This package contains a
client which will allow you to connect to other desktops running a VNC
server.

%package server
Summary: A TigerVNC server
Group: User Interface/X
Provides: vnc-server = 4.1.3-2, vnc-libs = 4.1.3-2
Obsoletes: vnc-server < 4.1.3-2, vnc-libs < 4.1.3-2
Provides: tightvnc-server = 1.5.0-0.15.20090204svn3586
Obsoletes: tightvnc-server < 1.5.0-0.15.20090204svn3586
Requires: perl
Requires: tigervnc-server-minimal
Requires: xorg-x11-xauth

%description server
The VNC system allows you to access the same desktop from a wide
variety of platforms.  This package includes set of utilities
which make usage of TigerVNC server more user friendly. It also
contains x0vncserver program which can export your active
X session.

%package server-minimal
Summary: A minimal installation of TigerVNC server
Group: User Interface/X
Requires(post): chkconfig
Requires(preun):chkconfig
Requires(preun):initscripts
Requires(postun):initscripts

Requires: xkeyboard-config, xorg-x11-xkb-utils
Requires: keyutils-libs-devel
Requires: tigervnc-license

%description server-minimal
The VNC system allows you to access the same desktop from a wide
variety of platforms. This package contains minimal installation
of TigerVNC server, allowing others to access the desktop on your
machine.

%package server-applet
Summary: Java TigerVNC viewer applet for TigerVNC server
Group: User Interface/X
Requires: tigervnc-server, java, jpackage-utils
%if 0%{?fedora} >= 10 || 0%{?rhel} >= 6 || 0%{?centos} >= 6
BuildArch: noarch
%endif

%description server-applet
The Java TigerVNC viewer applet for web browsers. Install this package to allow
clients to use web browser when connect to the TigerVNC server.

%package license
Summary: License of TigerVNC suite
Group: User Interface/X
%if 0%{?fedora} >= 10 || 0%{?rhel} >= 6 || 0%{?centos} >= 6
BuildArch: noarch
%endif

%description license
This package contains license of the TigerVNC suite

%package icons
Summary: Icons for TigerVNC viewer
Group: User Interface/X
%if 0%{?fedora} >= 10 || 0%{?rhel} >= 6 || 0%{?centos} >= 6
BuildArch: noarch
%endif

%description icons
This package contains icons for TigerVNC viewer

%if %{_bootstrap}
%package static-devel
Summary: Static development files necessary to build TigerVNC
Group: Development/Libraries

%description static-devel
This package contains static development files necessary to build TigerVNC
%endif

%prep
rm -rf $RPM_BUILD_ROOT
rm -rf %{_builddir}/%{name}-%{version}%{?snap:-%{snap}}
%setup -q -n %{name}-%{version}%{?snap:-%{snap}}

# Search paths for X11 are hard coded into FindX11.cmake
cp %SOURCE9 cmake/Modules/
sed -i -e "s#@_includedir@#%{xorg_buildroot}%{_includedir}#" cmake/Modules/FindX11.cmake
sed -i -e "s#@_libdir@#%{xorg_buildroot}%{_libdir}#" cmake/Modules/FindX11.cmake
%patch12 -p1 -b .static-build-fixes

%if %{_bootstrap}
tar xzf %SOURCE11
pushd fltk-*
%patch15 -p1 -b .static-libs
popd

tar xzf %SOURCE12
tar xjf %SOURCE13
tar xjf %SOURCE14
tar xzf %SOURCE15
tar xzf %SOURCE16
xzcat %SOURCE17 | tar xf -
%endif

mkdir xorg
pushd xorg
%if %{_bootstrap}
tar xjf %SOURCE100
tar xjf %SOURCE101
tar xjf %SOURCE102
tar xjf %SOURCE103
tar xjf %SOURCE104
tar xjf %SOURCE105
tar xjf %SOURCE106
tar xjf %SOURCE107
tar xjf %SOURCE108
tar xjf %SOURCE109
tar xjf %SOURCE110
tar xjf %SOURCE111
tar xjf %SOURCE112
tar xjf %SOURCE113
pushd libX11-*
%patch10006 -p1 -b .CVE-2013-7439
popd
tar xjf %SOURCE114
tar xjf %SOURCE115
tar xjf %SOURCE116
tar xjf %SOURCE117
tar xjf %SOURCE118
tar xjf %SOURCE119
tar xjf %SOURCE120
tar xjf %SOURCE121
tar xjf %SOURCE122
tar xjf %SOURCE123
pushd libXfont-*
%patch10003 -p1 -b .CVE-2015-1802
%patch10004 -p1 -b .CVE-2015-1803
%patch10005 -p1 -b .CVE-2015-1804
popd
tar xjf %SOURCE124
tar xjf %SOURCE125
tar xjf %SOURCE126
tar xjf %SOURCE127
tar xjf %SOURCE128
tar xjf %SOURCE129
tar xjf %SOURCE130
tar xjf %SOURCE131
tar xjf %SOURCE132
tar xjf %SOURCE133
tar xjf %SOURCE134
tar xjf %SOURCE135
tar xjf %SOURCE136
tar xjf %SOURCE137
tar xjf %SOURCE138
tar xjf %SOURCE139

tar xjf %SOURCE141
tar xjf %SOURCE142
tar xjf %SOURCE143
tar xjf %SOURCE144
tar xjf %SOURCE145
tar xjf %SOURCE146
tar xjf %SOURCE147
tar xjf %SOURCE148
tar xjf %SOURCE149
tar xjf %SOURCE150
tar xjf %SOURCE151
tar xjf %SOURCE152
tar xjf %SOURCE153
tar xjf %SOURCE154
tar xjf %SOURCE155
tar xjf %SOURCE156
tar xjf %SOURCE157
tar xjf %SOURCE158
%endif
tar xjf %SOURCE159
%if %{_bootstrap}
tar xjf %SOURCE160
tar xjf %SOURCE161
tar xjf %SOURCE162
tar xzf %SOURCE200
tar xjf %SOURCE201
pushd freetype-*
%patch46  -p1 -b .enable-valid
%patch88 -p1 -b .multilib
%patch89 -p1 -b .CVE-2010-2498
%patch90 -p1 -b .CVE-2010-2499
%patch91 -p1 -b .CVE-2010-2500
%patch92 -p1 -b .CVE-2010-2519
%patch93 -p1 -b .CVE-2010-2520
%patch96 -p1 -b .CVE-2010-1797
%patch97 -p1 -b .CVE-2010-2805
%patch98 -p1 -b .CVE-2010-2806
%patch99 -p1 -b .CVE-2010-2808
%patch100 -p1 -b .CVE-2010-3311
%patch101 -p1 -b .CVE-2010-3855
%patch102 -p1 -b .CVE-2011-0226
%patch103 -p1 -b .CVE-2011-3256
%patch104 -p1 -b .CVE-2011-3439
%patch105 -p1 -b .CVE-2012-1126
%patch106 -p1 -b .CVE-2012-1127
%patch107 -p1 -b .CVE-2012-1130
%patch108 -p1 -b .CVE-2012-1131
%patch109 -p1 -b .CVE-2012-1132
%patch110 -p1 -b .CVE-2012-1134
%patch111 -p1 -b .CVE-2012-1136
%patch112 -p1 -b .CVE-2012-1137
%patch113 -p1 -b .CVE-2012-1139
%patch114 -p1 -b .CVE-2012-1140
%patch115 -p1 -b .CVE-2012-1141
%patch116 -p1 -b .CVE-2012-1142
%patch117 -p1 -b .CVE-2012-1143
%patch118 -p1 -b .CVE-2012-1144
%patch119 -p1 -b .bdf-overflow
%patch120 -p1 -b .array-initialization
%patch121 -p1 -b .CVE-2012-5669
popd
tar xjf %SOURCE202
tar xjf %SOURCE203
tar xjf %SOURCE204
%endif
pushd xorg-server-1*
%patch10000 -p1 -b .CVE-2014-mult
%patch10001 -p1 -b .CVE-regressions
%patch10002 -p1 -b .CVE-2015-0255
for f in `find . -type f -perm -000`; do
  chmod +r "$f"
done
popd
%if %{_bootstrap}
tar xzf %SOURCE205
%endif
popd

cp -a xorg/xorg-server-1*/* unix/xserver

pushd unix/xserver
for all in `find . -type f -perm -001`; do
  chmod -x "$all"
done
patch -p1 < %{_builddir}/%{name}-%{version}%{?snap:-%{snap}}/unix/xserver112.patch
%patch13 -p1 -b .dridir
%patch14 -p1 -b .xkbcompdir

popd

%build
export CC=gcc44
export CXX=g++44
export CFLAGS="-g $RPM_OPT_FLAGS -fPIC"
export CXXFLAGS="$CFLAGS -static-libgcc"
export PYTHON=python26

%if %{_bootstrap}
mkdir -p %{xorg_buildroot}%{_libdir}
pushd %{xorg_buildroot}%{_libdir}
ln -s `g++44 -print-file-name=libz.a` .
ln -s `g++44 -print-file-name=libgcc.a` .
ln -s `g++44 -print-file-name=libexpat.a` .
ln -s `g++44 -print-file-name=libcrypto.a` .
popd

echo "*** Building libjpeg-turbo ***"
pushd libjpeg-turbo-*
LDFLAGS="$LDFLAGS -static" PKG_CONFIG="pkg-config --static" ./configure --host=%{_host} --build=%{_build} --prefix=%{_prefix} --libdir=%{_libdir} --disable-nls --enable-static --disable-shared
make %{?_smp_mflags} DESTDIR=%{xorg_buildroot} install
popd

echo "*** Building Xorg ***"
pushd xorg

echo "*** Building libsha1 ***"
pushd libsha1-*
autoreconf -fiv
LDFLAGS="$LDFLAGS -static" PKG_CONFIG="pkg-config --static" ./configure --host=%{_host} --build=%{_build} --prefix=%{_prefix} --libdir=%{_libdir} --disable-nls --enable-static --disable-shared
make %{?_smp_mflags} DESTDIR=%{xorg_buildroot} install
find %{xorg_buildroot}%{_prefix} -type f -name "*.la" -delete
find %{xorg_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|libdir=%{_libdir}|libdir=%{xorg_buildroot}%{_libdir}|" {} \;
find %{xorg_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|prefix=%{_prefix}|prefix=%{xorg_buildroot}%{_prefix}|" {} \;
popd
popd
%endif

export CFLAGS="$RPM_OPT_FLAGS -fPIC -I%{xorg_buildroot}%{_includedir}"
export CXXFLAGS="$RPM_OPT_FLAGS -fPIC -I%{xorg_buildroot}%{_includedir} -static-libgcc"
export CPPFLAGS=$CXXFLAGS
export LDFLAGS="-L%{xorg_buildroot}%{_libdir} -L%{xorg_buildroot}%{_libdir}/tigervnc $LDFLAGS"
export ACLOCAL="aclocal -I %{xorg_buildroot}%{_datadir}/aclocal"
export PKG_CONFIG_PATH="%{xorg_buildroot}%{_libdir}/pkgconfig:%{xorg_buildroot}%{_libdir}/tigervnc/pkgconfig:%{xorg_buildroot}%{_datadir}/pkgconfig:%{_libdir}/pkgconfig:%{_datadir}/pkgconfig"

%if %{_bootstrap}
echo "*** Building gmp ***"
pushd gmp-*
%ifarch x86_64 s390x ia64 ppc64 alpha sparc64
LDFLAGS="$LDFLAGS -static" PKG_CONFIG="pkg-config --static" ABI=64 ./configure --host=%{_host} --build=%{_build} --prefix=%{_prefix} --libdir=%{_libdir} --enable-static --disable-shared --enable-cxx
%else
LDFLAGS="$LDFLAGS -static" PKG_CONFIG="pkg-config --static" ABI=32 ./configure --host=%{_host} --build=%{_build} --prefix=%{_prefix} --libdir=%{_libdir} --enable-static --disable-shared --enable-cxx
%endif
make %{?_smp_mflags} DESTDIR=%{xorg_buildroot} install
find %{xorg_buildroot}%{_prefix} -type f -name "*.la" -delete
find %{xorg_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|libdir=%{_libdir}|libdir=%{xorg_buildroot}%{_libdir}|" {} \;
find %{xorg_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|prefix=%{_prefix}|prefix=%{xorg_buildroot}%{_prefix}|" {} \;
popd

echo "*** Building libtasn1 ***"
pushd libtasn1-*
LDFLAGS="$LDFLAGS -static" PKG_CONFIG="pkg-config --static" ./configure --host=%{_host} --build=%{_build} --prefix=%{_prefix} --libdir=%{_libdir} --enable-static --disable-shared
make %{?_smp_mflags} DESTDIR=%{xorg_buildroot} install
find %{xorg_buildroot}%{_prefix} -type f -name "*.la" -delete
find %{xorg_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|libdir=%{_libdir}|libdir=%{xorg_buildroot}%{_libdir}|" {} \;
find %{xorg_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|prefix=%{_prefix}|prefix=%{xorg_buildroot}%{_prefix}|" {} \;
popd

echo "*** Building nettle ***"
pushd nettle-*
autoreconf -fiv
LDFLAGS="$LDFLAGS -static" PKG_CONFIG="pkg-config --static" ./configure --host=%{_host} --build=%{_build} --prefix=%{_prefix} --libdir=%{_libdir} --enable-static --disable-shared --disable-openssl
make %{?_smp_mflags} DESTDIR=%{xorg_buildroot} install
find %{xorg_buildroot}%{_prefix} -type f -name "*.la" -delete
find %{xorg_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|libdir=%{_libdir}|libdir=%{xorg_buildroot}%{_libdir}|" {} \;
find %{xorg_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|prefix=%{_prefix}|prefix=%{xorg_buildroot}%{_prefix}|" {} \;
popd

echo "*** Building gnutls ***"
pushd gnutls-*
LDFLAGS="-L%{xorg_buildroot}%{_libdir} -lgmp $LDFLAGS -static" PKG_CONFIG="pkg-config --static" ./configure \
  --host=%{_host} \
  --build=%{_build} \
  --prefix=%{_prefix} \
  --libdir=%{_libdir} \
  --enable-static \
  --disable-shared \
  --without-p11-kit \
  --disable-guile \
  --disable-srp-authentication \
  --disable-libdane \
  --disable-doc \
  --enable-local-libopts \
  --without-tpm \
  --disable-dependency-tracking \
  --disable-silent-rules \
  --disable-heartbeat-support
make %{?_smp_mflags} DESTDIR=%{xorg_buildroot} install
find %{xorg_buildroot}%{_prefix} -type f -name "*.la" -delete
find %{xorg_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|libdir=%{_libdir}|libdir=%{xorg_buildroot}%{_libdir}|" {} \;
find %{xorg_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|prefix=%{_prefix}|prefix=%{xorg_buildroot}%{_prefix}|" {} \;
popd

pushd xorg

echo "*** Building freetype ***"
pushd freetype-*
LDFLAGS="$LDFLAGS -static" PKG_CONFIG="pkg-config --static" CFLAGS="$CFLAGS -fno-strict-aliasing" ./configure --host=%{_host} --build=%{_build} --prefix=%{_prefix} --libdir=%{_libdir} --enable-static --disable-shared
sed -i 's|^hardcode_libdir_flag_spec=.*|hardcode_libdir_flag_spec=""|g' builds/unix/libtool
sed -i 's|^runpath_var=LD_RUN_PATH|runpath_var=DIE_RPATH_DIE|g' builds/unix/libtool
make DESTDIR=%{xorg_buildroot} install
# FIXME: fontconfig bails out if we delete the libtool archives
find %{xorg_buildroot}%{_prefix} -type f -name "*.la" -exec sed -i -e "s|libdir='%{_libdir}'|libdir='%{xorg_buildroot}%{_libdir}'|" {} \;
find %{xorg_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|libdir=%{_libdir}|libdir=%{xorg_buildroot}%{_libdir}|" {} \;
find %{xorg_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|prefix=%{_prefix}|prefix=%{xorg_buildroot}%{_prefix}|" {} \;
# fix multilib issues
%ifarch x86_64 s390x ia64 ppc64 alpha sparc64
%define wordsize 64
%else
%define wordsize 32
%endif

mv %{xorg_buildroot}%{_includedir}/freetype2/freetype/config/ftconfig.h \
   %{xorg_buildroot}%{_includedir}/freetype2/freetype/config/ftconfig-%{wordsize}.h
cat >%{xorg_buildroot}%{_includedir}/freetype2/freetype/config/ftconfig.h <<EOF
#ifndef __FTCONFIG_H__MULTILIB
#define __FTCONFIG_H__MULTILIB

#include <bits/wordsize.h>

#if __WORDSIZE == 32
# include "ftconfig-32.h"
#elif __WORDSIZE == 64
# include "ftconfig-64.h"
#else
# error "unexpected value for __WORDSIZE macro"
#endif

#endif
EOF
popd

echo "*** Building fontconfig ***"
pushd fontconfig-*
autoreconf -fiv
LDFLAGS="$LDFLAGS -static" PKG_CONFIG="pkg-config --static" HASDOCBOOK=no ./configure --host=%{_host} --build=%{_build} --prefix=%{_prefix} --libdir=%{_libdir} --enable-static --disable-shared --with-confdir=%{_sysconfdir}/fonts --with-cache-dir=%{_localstatedir}/cache/fontconfig --with-default-fonts=%{_datadir}/fonts --with-add-fonts="%{_datadir}/X11/fonts/Type1,%{_datadir}/X11/fonts/OTF,%{_datadir}/X11/fonts/TTF,%{_datadir}/X11/fonts/misc,%{_datadir}/X11/fonts/100dpi,%{_datadir}/X11/fonts/75dpi,%{_prefix}/local/share/fonts,~/.fonts"
make %{?_smp_mflags}
make DESTDIR=%{xorg_buildroot} install
find %{xorg_buildroot}%{_prefix} -type f -name "*.la" -delete
find %{xorg_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|libdir=%{_libdir}|libdir=%{xorg_buildroot}%{_libdir}|" {} \;
find %{xorg_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|prefix=%{_prefix}|prefix=%{xorg_buildroot}%{_prefix}|" {} \;
popd

pushd util-macros-*
echo "Building macros"
LDFLAGS="$LDFLAGS -static" PKG_CONFIG="pkg-config --static" ./configure --host=%{_host} --build=%{_build} --prefix=%{_prefix} --libdir=%{_libdir} --enable-static --disable-shared
make DESTDIR=%{xorg_buildroot} install
find %{xorg_buildroot}%{_prefix} -type f -name "*.la" -delete
find %{xorg_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|libdir=%{_libdir}|libdir=%{xorg_buildroot}%{_libdir}|" {} \;
find %{xorg_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|prefix=%{_prefix}|prefix=%{xorg_buildroot}%{_prefix}|" {} \;
popd

modules="\
    bigreqsproto \
    compositeproto \
    damageproto \
    dri2proto \
    fixesproto \
    fontsproto \
    glproto \
    inputproto \
    kbproto \
    randrproto \
    recordproto \
    renderproto \
    resourceproto \
    scrnsaverproto \
    videoproto \
    xcb-proto \
    xproto \
    xcmiscproto \
    xextproto \
    xf86bigfontproto \
    xf86dgaproto \
    xf86driproto \
    xf86vidmodeproto \
    xineramaproto \
    makedepend \
    xtrans \
    libXau \
    libXdmcp \
    libxcb \
    libX11 \
    libXext \
    libfontenc \
    libICE \
    libSM \
    libXt \
    libXmu \
    libXpm \
    libXaw \
    libXfixes \
    libXcomposite \
    libXrender \
    libXdamage \
    libXcursor \
    libXfont \
    libXft \
    libXi \
    libXinerama \
    libxkbfile \
    libXrandr \
    libXres \
    libXScrnSaver \
    libXtst \
    libXv \
    libXxf86dga \
    libXxf86vm \
    libpciaccess \
    pixman \
    libdrm \
    font-util"

for module in ${modules}; do
  extraoptions=""
  pushd ${module}-*
  echo ======================
  echo configuring ${module}
  echo ======================
%ifarch i386 i686 x86_64
  if [ "${module}" = "libdrm" ]; then
    autoreconf -fiv
    extraoptions="${extraoptions} --enable-udev --disable-libkms --disable-manpages --disable-intel --disable-radeon --disable-nouveau --disable-vmwgfx"
  fi
%endif
  if [ "${module}" = "libXdmcp" ]; then
    autoreconf -fiv
  fi
  if [ "${module}" = "libXcursor" ]; then
    autoreconf -fiv
  fi
  if [ "${module}" = "libfontenc" ]; then
    autoconf
  fi
  if [ "${module}" = "libXi" ]; then
    autoreconf -fiv
  fi
  if [ "${module}" = "libXaw" ]; then
    extraoptions="${extraoptions} --disable-xaw8 --disable-xaw6"
  fi
  if [ "${module}" = "libxcb" ]; then
    sed -i 's/pthread-stubs //' configure.ac
    autoreconf -fiv
  fi
  if [ "${module}" = "libX11" ]; then
    autoreconf -fiv
    extraoptions="${extraoptions} --disable-specs"
  fi
  if [ "${module}" = "libSM" ]; then
    extraoptions="${extraoptions} --without-libuuid"
  fi
  if [ "${module}" = "pixman" ]; then
    extraoptions="${extraoptions} --disable-gtk --disable-openmp"
    aclocal -I %{xorg_buildroot}%{_datadir}/aclocal
    autoconf
    autoreconf -fiv
  fi
  if [ "${module}" = "libXfont" ]; then
    extraoptions="${extraoptions} --with-freetype-config=%{xorg_buildroot}%{_bindir}/freetype-config"
  fi
  if [ "${module}" = "libXScrnSaver" ]; then
    LDFLAGS="$LDFLAGS -static" PKG_CONFIG="pkg-config --static" CFLAGS="$CFLAGS -fno-strict-aliasing" ./configure --host=%{_host} --build=%{_build} --prefix=%{_prefix} --libdir=%{_libdir} ${extraoptions} --enable-static --disable-shared --with-pic
  elif [ "${module}" = "libxkbfile" ]; then
    LDFLAGS="$LDFLAGS -static" PKG_CONFIG="pkg-config --static" CFLAGS="$CFLAGS -fno-strict-aliasing" ./configure --host=%{_host} --build=%{_build} --prefix=%{_prefix} --libdir=%{_libdir} ${extraoptions} --enable-static --disable-shared --with-pic
  elif [ "${module}" = "pixman" ]; then
    LDFLAGS="$LDFLAGS -static" PKG_CONFIG="pkg-config --static" CFLAGS="$CFLAGS -fno-strict-aliasing" ./configure --host=%{_host} --build=%{_build} --prefix=%{_prefix} --libdir=%{_libdir} ${extraoptions} --enable-static --disable-shared --with-pic
  elif [ "${module}" = "libXt" ]; then
    LDFLAGS="$LDFLAGS -static" PKG_CONFIG="pkg-config --static" CFLAGS="$CFLAGS -fno-strict-aliasing" ./configure --host=%{_host} --build=%{_build} --prefix=%{_prefix} --libdir=%{_libdir} ${extraoptions} --enable-static --disable-shared --with-pic --with-xfile-search-path="%{_sysconfdir}/X11/%%L/%%T/%%N%%C%%S:%{_sysconfdir}/X11/%%l/%%T/\%%N%%C%%S:%{_sysconfdir}/X11/%%T/%%N%%C%%S:%{_sysconfdir}/X11/%%L/%%T/%%N%%S:%{_sysconfdir}/X\11/%%l/%%T/%%N%%S:%{_sysconfdir}/X11/%%T/%%N%%S:%{_datadir}/X11/%%L/%%T/%%N%%C%%S:%{_datadir}/X1\1/%%l/%%T/%%N%%C%%S:%{_datadir}/X11/%%T/%%N%%C%%S:%{_datadir}/X11/%%L/%%T/%%N%%S:%{_datadir}/X11/%%\l/%%T/%%N%%S:%{_datadir}/X11/%%T/%%N%%S"
  elif [ "${module}" = "libX11" ]; then
    LDFLAGS="$LDFLAGS -lpthread" PKG_CONFIG="pkg-config --static" ./configure --host=%{_host} --build=%{_build} --prefix=%{_prefix} --libdir=%{_libdir} ${extraoptions} --disable-static --enable-shared --with-pic
  elif [ "${module}" = "libXtst" ]; then
    LDFLAGS="$LDFLAGS -static" PKG_CONFIG="pkg-config --static" ./configure --host=%{_host} --build=%{_build} --prefix=%{_prefix} --libdir=%{_libdir} ${extraoptions} --enable-static --disable-shared --with-pic
  elif [ "${module}" = "libXpm" ]; then
    LDFLAGS="$LDFLAGS -static" PKG_CONFIG="pkg-config --static" ./configure --host=%{_host} --build=%{_build} --prefix=%{_prefix} --libdir=%{_libdir} ${extraoptions} --enable-static --disable-shared --with-pic
  else
    LDFLAGS="$LDFLAGS -static" PKG_CONFIG="pkg-config --static" ./configure --host=%{_host} --build=%{_build} --prefix=%{_prefix} --libdir=%{_libdir} ${extraoptions} --enable-static --disable-shared --with-pic
  fi
  echo ======================
  echo building ${module}
  echo ======================
  make DESTDIR=%{xorg_buildroot}
  if [ "${module}" = "libX11" ]; then
    make DESTDIR=%{xorg_buildroot} INSTALL="install -p" install
  else
    make DESTDIR=%{xorg_buildroot} install
  fi
  find %{xorg_buildroot}%{_prefix} -type f -name "*.la" -delete
  find %{xorg_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|libdir=%{_libdir}|libdir=%{xorg_buildroot}%{_libdir}|" {} \;
  find %{xorg_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|prefix=%{_prefix}|prefix=%{xorg_buildroot}%{_prefix}|" {} \;
  if [ "${module}" = "libxcb" ]; then
    sed 's,@libdir@,%{xorg_buildroot}%{_libdir},;s,@prefix@,%{xorg_buildroot}%{_prefix},;s,@exec_prefix@,%{xorg_buildroot}%{_exec_prefix},' %{SOURCE140} > %{xorg_buildroot}%{_libdir}/pkgconfig/pthread-stubs.pc
    sed -i -e 's/^\(Libs.private:.*\)$/\1 -L${libdir} -lXdmcp -lXau/' %{xorg_buildroot}%{_libdir}/pkgconfig/xcb.pc
  elif [ "${module}" = "libX11" ]; then
    sed -i -e 's/^\(Libs:.*\)$/\1 -ldl/' %{xorg_buildroot}%{_libdir}/pkgconfig/x11.pc
    sed -i -e 's/^\(Libs.private:.*\)$/\1 -L${libdir} -lxcb/' %{xorg_buildroot}%{_libdir}/pkgconfig/x11.pc
  elif [ "${module}" = "libSM" ]; then
    echo 'Libs.private: -L${libdir} -lICE' >> %{xorg_buildroot}%{_libdir}/pkgconfig/sm.pc
  fi

  popd
done
%else
pushd xorg
%endif

%if %{_bootstrap}
# build mesa
echo "*** Building Mesa ***"
pushd Mesa-*
export PYTHON2=python26
%ifarch %{ix86}
sed -i -e 's/-std=c99/-std=gnu99/g' configure.ac
%endif
autoreconf -fiv
%ifarch %{ix86}
# i do not have words for how much the assembly dispatch code infuriates me
%define common_flags --disable-selinux --enable-pic --disable-asm
%else
%define common_flags --disable-selinux --enable-pic
%endif

# link libGL statically against any xorg libraries built above
LDFLAGS="$LDFLAGS -Wl,-Bstatic -lxcb -lXdmcp -lXau -lXext -lXxf86vm -ldrm -Wl,-Bdynamic -lX11 -lpthread -Wl,-rpath,"'\$$'"ORIGIN/../..%{_libdir}/tigervnc:%{_libdir}/tigervnc:%{_libdir}" \
PKG_CONFIG="pkg-config --static" ./configure %{common_flags} \
    --host=%{_host} \
    --build=%{_build} \
    --prefix=%{_prefix} \
    --libdir=%{_libdir}/tigervnc \
    --disable-osmesa \
    --disable-shared-glapi \
    --disable-egl \
    --disable-gbm \
    --enable-glx \
    --disable-glx-tls \
    --disable-opencl \
    --disable-xvmc \
    --with-dri-driverdir=%{_libdir}/tigervnc/dri \
    --disable-gallium-egl \
    --with-gallium-drivers="" \
    --with-dri-drivers=swrast

make DESTDIR=%{xorg_buildroot}
make install DESTDIR=%{xorg_buildroot}
find %{xorg_buildroot}%{_prefix} -type f -name "*.la" -delete
find %{xorg_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|libdir=%{_libdir}|libdir=%{xorg_buildroot}%{_libdir}|" {} \;
find %{xorg_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|prefix=%{_prefix}|prefix=%{xorg_buildroot}%{_prefix}|" {} \;
popd
%endif

popd

%if %{_bootstrap}
echo "*** Building libpng ***"
pushd libpng-*
CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" LDFLAGS="${LDFLAGS}" ./configure \
  --host=%{_host} \
  --build=%{_build} \
  --prefix=%{_prefix} \
  --libdir=%{_libdir} \
  --disable-shared \
  --enable-static
make %{?_smp_mflags}
make DESTDIR=%{xorg_buildroot} install
popd

echo "*** Building fltk ***"
pushd fltk-*
%endif
export CMAKE_PREFIX_PATH="%{xorg_buildroot}%{_prefix}:%{_prefix}"
export CMAKE_EXE_LINKER_FLAGS=$LDFLAGS
export PKG_CONFIG="pkg-config --static"
%if %{_bootstrap}
./configure \
  --host=%{_host} \
  --build=%{_build} \
  --prefix=%{_prefix} \
  --libdir=%{_libdir} \
  --enable-x11 \
  --enable-gl \
  --disable-shared \
  --enable-localjpeg \
  --enable-localzlib \
  --disable-localpng \
  --enable-xinerama \
  --enable-xft \
  --enable-xdbe \
  --enable-xfixes \
  --enable-xcursor \
  --with-x
make %{?_smp_mflags}
make DESTDIR=%{xorg_buildroot} install
popd
%endif

echo "*** Building VNC ***"
export CFLAGS="$CFLAGS -fPIC"
export CXXFLAGS=`echo $CXXFLAGS | sed -e 's/ -c //g'`
%{cmake28} -G"Unix Makefiles" \
  -DFLTK_FLUID_EXECUTABLE=%{xorg_buildroot}%{_bindir}/fluid \
  -DFLTK_LIBRARY_DIR=%{xorg_buildroot}%{_libdir} \
  -DFLTK_INCLUDE_DIR=%{xorg_buildroot}%{_includedir} \
  -DBUILD_STATIC=1 \
  -DCMAKE_BUILD_TYPE=Release \
  -DUSE_INCLUDED_ZLIB=0 \
  -DZLIB_INCLUDE_DIR=%{_includedir} \
  -DZLIB_LIBRARY=%{_libdir}/libz.a \
  -DCMAKE_INSTALL_PREFIX=%{_prefix}

make %{?_smp_mflags}

pushd unix/xserver
export PIXMANINCDIR=%{xorg_buildroot}%{_includedir}/pixman-1
sed -i -e 's/^\(\s*WAYLAND_SCANNER_RULES.*\)/dnl\1/' configure.ac
autoreconf -fiv
chmod +x ./configure
# create a relocatable Xvnc so that we can bundle the custom libGL & swrast w/o overwriting existing libs
GL_LIBS='-Wl,-Bdynamic -lGL' LDFLAGS="$LDFLAGS -Wl,-rpath,"'\$$'"ORIGIN/../..%{_libdir}/tigervnc:%{_libdir}/tigervnc:%{_libdir}" \
%configure \
  --prefix=%{_prefix} --libdir=%{_libdir} --mandir=%{_datadir}/man \
  --sysconfdir=%{_sysconfdir} --localstatedir=%{_localstatedir} \
  --with-vendor-name="The TigerVNC Project" --with-vendor-name-short="TigerVNC" \
  --with-vendor-web="http://www.tigervnc.org" \
  --disable-xorg --disable-xnest --disable-xvfb --disable-dmx \
  --disable-xwin --disable-xephyr --disable-kdrive --disable-wayland \
  --with-pic --enable-static --disable-shared --enable-xinerama \
  --with-default-xkb-rules=base \
  --with-default-font-path="catalogue:%{_sysconfdir}/X11/fontpath.d,%{_datadir}/X11/fonts/misc,%{_datadir}/X11/fonts/OTF,%{_datadir}/X11/fonts/TTF,%{_datadir}/X11/fonts/Type1,%{_datadir}/X11/fonts/100dpi,%{_datadir}/X11/fonts/75dpi,built-ins" \
  --with-serverconfig-path=%{_libdir}/xorg \
  --with-fontrootdir=%{_datadir}/X11/fonts \
  --with-xkb-output=%{_localstatedir}/lib/xkb \
  --enable-install-libxf86config \
  --enable-glx --disable-glx-tls --disable-dri --enable-dri2 --disable-dri3 \
  --disable-present \
  --disable-config-dbus \
  --disable-config-hal \
  --disable-config-udev \
  --without-dtrace \
  --disable-unit-tests \
  --disable-docs \
  --disable-devel-docs \
  --disable-selective-werror \
  --with-sha1=libsha1

make TIGERVNC_SRCDIR=%{tigervnc_src_dir} %{?_smp_mflags}
popd

# Build icons
pushd media
make
popd

# Build Java applet
pushd java
%{cmake28} \
%if !%{_self_signed}
  -DJAVA_KEYSTORE=%{_keystore} \
  -DJAVA_KEYSTORE_TYPE=%{_keystore_type} \
  -DJAVA_KEY_ALIAS=%{_key_alias} \
  -DJAVA_STOREPASS=":env STOREPASS" \
  -DJAVA_KEYPASS=":env KEYPASS" \
  -DJAVA_TSA_URL=https://timestamp.geotrust.com/tsa .
%endif

JAVA_TOOL_OPTIONS="-Dfile.encoding=UTF8" make
popd

%install
make install DESTDIR=$RPM_BUILD_ROOT

pushd unix/xserver/hw/vnc
make install DESTDIR=$RPM_BUILD_ROOT
popd

mkdir -p $RPM_BUILD_ROOT%{_libdir}/tigervnc/dri
install -m644 -p %{xorg_buildroot}%{_libdir}/tigervnc/dri/swrast_dri.so $RPM_BUILD_ROOT%{_libdir}/tigervnc/dri
for f in `find %{xorg_buildroot}%{_libdir}/tigervnc -name "lib*" -print` ; do
cp -a $f $RPM_BUILD_ROOT%{_libdir}/tigervnc
done

mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/init.d
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/sysconfig
install -m644 %{SOURCE1} $RPM_BUILD_ROOT%{_sysconfdir}/init.d/vncserver
install -m644 %{SOURCE2} $RPM_BUILD_ROOT%{_sysconfdir}/sysconfig/vncservers

# Install Java applet
pushd java
mkdir -p $RPM_BUILD_ROOT%{_datadir}/vnc/classes
install -m755 VncViewer.jar $RPM_BUILD_ROOT%{_datadir}/vnc/classes
install -m644 com/tigervnc/vncviewer/index.vnc $RPM_BUILD_ROOT%{_datadir}/vnc/classes
popd

%find_lang %{name} %{name}.lang

%if %{_bootstrap}
find %{xorg_buildroot}%{_prefix} -type f -name "*.la" -delete
find %{xorg_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|libdir=%{xorg_buildroot}%{_libdir}|libdir=/opt/%{name}/%{scl_name}%{_libdir}|" {} \;
find %{xorg_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|prefix=%{xorg_buildroot}%{_prefix}|prefix=/opt/%{name}/%{scl_name}%{_prefix}|" {} \;
%endif

# remove unwanted files
rm -rf $RPM_BUILD_ROOT%{_libdir}/tigervnc/pkgconfig
rm -rf $RPM_BUILD_ROOT%{_libdir}/pkgconfig
rm -rf $RPM_BUILD_ROOT%{_libdir}/xorg
rm -rf $RPM_BUILD_ROOT%{_includedir}
rm -f $RPM_BUILD_ROOT%{_libdir}/tigervnc/*.la
rm -f $RPM_BUILD_ROOT%{_libdir}/tigervnc/dri/*.la

%clean
rm -rf $RPM_BUILD_ROOT

%post
touch -c %{_datadir}/icons/hicolor
if [ -x %{_bindir}/gtk-update-icon-cache ]; then
	%{_bindir}/gtk-update-icon-cache -q %{_datadir}/icons/hicolor || :
fi

%postun
touch -c %{_datadir}/icons/hicolor
if [ -x %{_bindir}/gtk-update-icon-cache ]; then
	%{_bindir}/gtk-update-icon-cache -q %{_datadir}/icons/hicolor || :
fi

%post server
/sbin/chkconfig --add vncserver

%preun server
if [ $1 -eq 0 ]; then
	/sbin/service vncserver stop &>/dev/null || :
	/sbin/chkconfig --del vncserver
fi

%postun server
/sbin/service vncserver condrestart &>/dev/null || :

%files -f %{name}.lang
%defattr(-,root,root,-)
%doc README.rst
%{_bindir}/vncviewer
%{_datadir}/applications/*
%{_mandir}/man1/vncviewer.1*

%files server
%defattr(-,root,root,-)
%config(noreplace) %{_sysconfdir}/sysconfig/vncservers
%config(noreplace) %{_sysconfdir}/init.d/vncserver
%{_bindir}/x0vncserver
%{_bindir}/vncserver
%{_mandir}/man1/vncserver.1*
%{_mandir}/man1/x0vncserver.1*

%files server-minimal
%defattr(-,root,root,-)
%{_bindir}/vncconfig
%{_bindir}/vncpasswd
%{_bindir}/Xvnc
%{_mandir}/man1/Xvnc.1*
%{_mandir}/man1/vncpasswd.1*
%{_mandir}/man1/vncconfig.1*
%{_libdir}/*

%files server-applet
%defattr(-,root,root,-)
%doc java/com/tigervnc/vncviewer/README
%{_datadir}/vnc/classes/*

%files license
%defattr(-,root,root,-)
%doc LICENCE.TXT

%files icons
%defattr(-,root,root,-)
%{_datadir}/icons/hicolor/*/apps/*

%if %{_bootstrap}
%files static-devel
%defattr(-,root,root,-)
/opt/%{name}/%{scl_name}%{_sysconfdir}/*
/opt/%{name}/%{scl_name}%{_bindir}/*
/opt/%{name}/%{scl_name}%{_datadir}/*
/opt/%{name}/%{scl_name}%{_includedir}/*
/opt/%{name}/%{scl_name}%{_libdir}/*
%ifarch x86_64 s390x ia64 ppc64 alpha sparc64
/opt/%{name}/%{scl_name}%{_prefix}/lib/python2.6/*
%endif
%endif

%changelog
* Fri Dec 11 2015 Brian P. Hinz <bphinz@users.sourceforge.net> 1.6.80-3
- Configure with --host and --build to avoid build host-specific compiler opts

* Fri Nov 27 2015 Brian P. Hinz <bphinz@users.sourceforge.net> 1.6.80-2
- Split static pre-reqs into separate package

* Thu Nov 26 2015 Brian P. Hinz <bphinz@users.sourceforge.net> 1.6.80-1
- Version bump for 1.6 release
- Update libjpeg-turbo, gnutls, libtasn1, libpng to latest upstream versions.

* Sun Sep 12 2015 Brian P. Hinz <bphinz@users.sourceforge.net> 1.5.80-8
- Build Xvnc with link path relative to $ORIGIN and apply dridir patch
  to make it portable.

* Sun Aug 09 2015 Brian P. Hinz <bphinz@users.sourceforge.net> 1.5.80-7
- Patch Xorg sources with latest relevant CVE patches.
- Update libjpeg-turbo, gnutls, libtasn1 to latest upstream versions.

* Sat Mar 14 2015 Brian P. Hinz <bphinz@users.sourceforge.net> 1.4.80-6
- Build static libraries to meet new minimum requirements

* Sat Mar 07 2015 Brian P. Hinz <bphinz@users.sourceforge.net> 1.4.80-5
- Don't disable xinerama extension

* Thu Feb 19 2015 Brian P. Hinz <bphinz@users.sourceforge.net> 1.4.80-4
- Bumped fltk to 1.3.3, no longer requires patching

* Mon Jan 19 2015 Brian P. Hinz <bphinz@users.sourceforge.net> 1.4.0-3
- Added default font paths to Xvnc and fontconfig
- Added vendor strings to Xvnc
- Specified xfile-search-path when configuring libXt the same way el6 does

* Wed Dec 24 2014 Brian P. Hinz <bphinz@users.sourceforge.net> 1.4.80-1.20141119git59c5a55c
- Rebuilt against Xorg 7.7 with CVE-2104-12-09 patches from debian.
- Bumped versions of Mesa, Freetype, fontconfig, etc.
- Link against our own version of libGL to improve portability.
- Added static libsha1 to avoid linking against libssl.so.

* Wed Nov 19 2014 Brian P. Hinz <bphinz@users.sourceforge.net> 1.3.80-18.20141119git59c5a55c
- Removed server module sub-package

* Thu Nov 28 2013 Brian P. Hinz <bphinz@users.sourceforge.net> 1.3.80-17.20131128svn5139
- Bumped version to 1.3.80
- Cleaned up linter warnings

* Thu Jul 05 2013 Brian P. Hinz <bphinz@users.sourceforge.net> 1.3.0
- Upstream 1.3.0 release
- Conditional-ized %snap for release

* Fri Jun 14 2013 Brian P. Hinz <bphinz@users.sourceforge.net> 1.2.90-14.20130531svn5120
- Update libjpeg-turbo to 1.3.0

* Fri May 24 2013 Brian P. Hinz <bphinz@users.sourceforge.net> 1.2.90-14.20130524svn5114
- Improve spec file portability

* Fri May 17 2013 Brian P. Hinz <bphinz@users.sourceforge.net> 1.2.90-13.20130425svn5087
- Improve portability with more static linking

* Thu Apr 04 2013 Brian P. Hinz <bphinz@users.sourceforge.net> 1.2.80-12.20130330svn5066
- Added conditional -march arg to libdrm-intel to allow building on i386
- Fixed version to reflect upstream pre-release versioning

* Sat Mar 30 2013 Brian P. Hinz <bphinz@users.sourceforge.net> 1.2.0-11.20130330svn5066
- Updated to TigerVNC svn 5066
- Updated fltk to 1.3.2 and updated fltk patches per BUILDING.txt
- Fixed vncserver init script & config file which had been overwritten by
  systemd versions.

* Wed Nov 28 2012 Brian P. Hinz <bphinz@users.sourceforge.net> 1.2.0-7.20120915svn4999
- Changed BuildRequires to cmake28
- Set PIXMANINCDIR when building Xvnc

* Tue Sep 18 2012 Brian P. Hinz <bphinz@users.sourceforge.net> 1.2.0-6.20120915svn4999
- Applied icon support patch

* Sat Sep 15 2012 Brian P. Hinz <bphinz@users.sourceforge.net> 1.2.0-5.20120915svn4999
- Update to TigerVNC svn r4999 snapshot
- Build a static libjpeg-turbo to remove the external dependency
- Applied Cendio's Fltk patches, except for the icon patch which I cannot get to build
  without creating undefined reference errors during linking

* Thu Jul 19 2012 Brian P. Hinz <bphinz@users.sourceforge.net> 1.2.0-4.20120719svn4941
- Update to TigerVNC svn r4941 snapshot
- Removed border-hook.patch since it's been committed

* Wed Jul 18 2012 Brian P. Hinz <bphinz@users.sourceforge.net> 1.2.0-3.20120715svn4937
- Update to TigerVNC svn r4937 snapshot
- Applied border-hook.patch from devel list to fix bug #3415308
- Use build order recommended by cgit.freedesktop.org/xorg/util/modular/tree/build.sh
- Removed tigervnc11-rh692048.patch as it seems to break support for VeNCrypt

* Sun Jul 15 2012 Brian P. Hinz <bphinz@users.sourceforge.net> 1.2.0-1.20120715svn4935
- Adapted spec file for building static linked binary on RHEL5 from F16
  spec file and DRC's build-xorg script included in src tarball.
- Update to TigerVNC svn r4935 snapshot
- Need to use inkscape on RHEL5 because convert is broken

* Tue Nov 22 2011 Adam Tkac <atkac redhat com> - 1.1.0-3
- don't build X.Org devel docs (#755782)
- applet: BR generic java-devel instead of java-gcj-devel (#755783)
- use runuser to start Xvnc in systemd service file (#754259)
- don't attepmt to restart Xvnc session during update/erase (#753216)

* Fri Nov 11 2011 Adam Tkac <atkac redhat com> - 1.1.0-2
- libvnc.so: don't use unexported GetMaster function (#744881)
- remove nasm buildreq

* Mon Sep 12 2011 Adam Tkac <atkac redhat com> - 1.1.0-1
- update to 1.1.0
- update the xorg11 patch
- patches merged
  - tigervnc11-glx.patch
  - tigervnc11-CVE-2011-1775.patch
  - 0001-Use-memmove-instead-of-memcpy-in-fbblt.c-when-memory.patch

* Thu Jul 28 2011 Adam Tkac <atkac redhat com> - 1.0.90-6
- add systemd service file and remove legacy SysV initscript (#717227)

* Tue May 12 2011 Adam Tkac <atkac redhat com> - 1.0.90-5
- make Xvnc buildable against X.Org 1.11

* Tue May 10 2011 Adam Tkac <atkac redhat com> - 1.0.90-4
- viewer can send password without proper validation of X.509 certs
  (CVE-2011-1775)

* Wed Apr 13 2011 Adam Tkac <atkac redhat com> - 1.0.90-3
- fix wrong usage of memcpy which caused screen artifacts (#652590)
- don't point to inaccessible link in sysconfig/vncservers (#644975)

* Fri Apr 08 2011 Adam Tkac <atkac redhat com> - 1.0.90-2
- improve compatibility with vinagre client (#692048)

* Tue Mar 22 2011 Adam Tkac <atkac redhat com> - 1.0.90-1
- update to 1.0.90

* Wed Feb 09 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.0.90-0.32.20110117svn4237
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Mon Jan 17 2011 Adam Tkac <atkac redhat com> 1.0.90-0.31.20110117svn4237
- fix libvnc.so module loading

* Mon Jan 17 2011 Adam Tkac <atkac redhat com> 1.0.90-0.30.20110117svn4237
- update to r4237
- patches merged
  - tigervnc11-optionsdialog.patch
  - tigervnc11-rh607866.patch

* Fri Jan 14 2011 Adam Tkac <atkac redhat com> 1.0.90-0.29.20101208svn4225
- improve patch for keyboard issues

* Fri Jan 14 2011 Adam Tkac <atkac redhat com> 1.0.90-0.28.20101208svn4225
- attempt to fix various keyboard-related issues (key repeating etc)

* Fri Jan 07 2011 Adam Tkac <atkac redhat com> 1.0.90-0.27.20101208svn4225
- render "Ok" and "Cancel" buttons in the options dialog correctly

* Wed Dec 15 2010 Jan Görig <jgorig redhat com> 1.0.90-0.26.20101208svn4225
- added vncserver lock file (#662784)

* Fri Dec 10 2010 Adam Tkac <atkac redhat com> 1.0.90-0.25.20101208svn4225
- update to r4225
- patches merged
  - tigervnc11-rh611677.patch
  - tigervnc11-rh633931.patch
  - tigervnc11-xorg1.10.patch
- enable VeNCrypt and PAM support

* Mon Dec 06 2010 Adam Tkac <atkac redhat com> 1.0.90-0.24.20100813svn4123
- rebuild against xserver 1.10.X
- 0001-Return-Success-from-generate_modkeymap-when-max_keys.patch merged

* Wed Sep 29 2010 jkeating - 1.0.90-0.23.20100813svn4123
- Rebuilt for gcc bug 634757

* Tue Sep 21 2010 Adam Tkac <atkac redhat com> 1.0.90-0.22.20100420svn4030
- drop xorg-x11-fonts-misc dependency (#636170)

* Tue Sep 21 2010 Adam Tkac <atkac redhat com> 1.0.90-0.21.20100420svn4030
- improve patch for #633645 (fix tcsh incompatibilities)

* Thu Sep 16 2010 Adam Tkac <atkac redhat com> 1.0.90-0.20.20100813svn4123
- press fake modifiers correctly (#633931)
- supress unneeded debug information emitted from initscript (#633645)

* Wed Aug 25 2010 Adam Tkac <atkac redhat com> 1.0.90-0.19.20100813svn4123
- separate Xvnc, vncpasswd and vncconfig to -server-minimal subpkg (#626946)
- move license to separate subpkg and Requires it from main subpkgs
- Xvnc: handle situations when no modifiers exist well (#611677)

* Fri Aug 13 2010 Adam Tkac <atkac redhat com> 1.0.90-0.18.20100813svn4123
- update to r4123 (#617973)
- add perl requires to -server subpkg (#619791)

* Thu Jul 22 2010 Adam Tkac <atkac redhat com> 1.0.90-0.17.20100721svn4113
- update to r4113
- patches merged
  - tigervnc11-rh586406.patch
  - tigervnc11-libvnc.patch
  - tigervnc11-rh597172.patch
  - tigervnc11-rh600070.patch
  - tigervnc11-options.patch
- don't own %%{_datadir}/icons directory (#614301)
- minor improvements in the .desktop file (#616340)
- bundled libjpeg configure requires nasm; is executed even if system-wide
  libjpeg is used

* Fri Jul 02 2010 Adam Tkac <atkac redhat com> 1.0.90-0.16.20100420svn4030
- build against system-wide libjpeg-turbo (#494458)
- build no longer requires nasm

* Mon Jun 28 2010 Adam Tkac <atkac redhat com> 1.0.90-0.15.20100420svn4030
- vncserver: accept <+optname> option when specified as the first one

* Thu Jun 24 2010 Adam Tkac <atkac redhat com> 1.0.90-0.14.20100420svn4030
- fix memory leak in Xvnc input code (#597172)
- don't crash when receive negative encoding (#600070)
- explicitly disable udev configuration support
- add gettext-autopoint to BR

* Mon Jun 14 2010 Adam Tkac <atkac redhat com> 1.0.90-0.13.20100420svn4030
- update URL about SSH tunneling in the sysconfig file (#601996)

* Fri Jun 11 2010 Adam Tkac <atkac redhat com> 1.0.90-0.12.20100420svn4030
- use newer gettext
- autopoint now uses git instead of cvs, adjust BuildRequires appropriately

* Thu May 13 2010 Adam Tkac <atkac redhat com> 1.0.90-0.11.20100420svn4030
- link libvnc.so "now" to catch "undefined symbol" errors during Xorg startup
- use always XkbConvertCase instead of XConvertCase (#580159, #586406)
- don't link libvnc.so against libXi.la, libdix.la and libxkb.la; use symbols
  from Xorg instead

* Thu May 13 2010 Adam Tkac <atkac redhat com> 1.0.90-0.10.20100420svn4030
- update to r4030 snapshot
- patches merged to upstream
  - tigervnc11-rh522369.patch
  - tigervnc11-rh551262.patch
  - tigervnc11-r4002.patch
  - tigervnc11-r4014.patch

* Thu Apr 08 2010 Adam Tkac <atkac redhat com> 1.0.90-0.9.20100219svn3993
- add server-applet subpackage which contains Java vncviewer applet
- fix Java applet; it didn't work when run from web browser
- add xorg-x11-xkb-utils to server Requires

* Fri Mar 12 2010 Adam Tkac <atkac redhat com> 1.0.90-0.8.20100219svn3993
- add French translation to vncviewer.desktop (thanks to Alain Portal)

* Thu Mar 04 2010 Adam Tkac <atkac redhat com> 1.0.90-0.7.20100219svn3993
- don't crash during pixel format change (#522369, #551262)

* Mon Mar 01 2010 Adam Tkac <atkac redhat com> 1.0.90-0.6.20100219svn3993
- add mesa-dri-drivers and xkeyboard-config to -server Requires
- update to r3993 1.0.90 snapshot
  - tigervnc11-noexecstack.patch merged
  - tigervnc11-xorg18.patch merged
  - xserver18.patch is no longer needed

* Wed Jan 27 2010 Jan Gorig <jgorig redhat com> 1.0.90-0.5.20091221svn3929
- initscript LSB compliance fixes (#523974)

* Fri Jan 22 2010 Adam Tkac <atkac redhat com> 1.0.90-0.4.20091221svn3929
- mark stack as non-executable in jpeg ASM code
- add xorg-x11-xauth to Requires
- add support for X.Org 1.8
- drop shave sources, they are no longer needed

* Thu Jan 21 2010 Adam Tkac <atkac redhat com> 1.0.90-0.3.20091221svn3929
- drop tigervnc-xorg25909.patch, it has been merged to X.Org upstream

* Thu Jan 07 2010 Adam Tkac <atkac redhat com> 1.0.90-0.2.20091221svn3929
- add patch for upstream X.Org issue #25909
- add libXdmcp-devel to build requires to build Xvnc with XDMCP support (#552322)

* Mon Dec 21 2009 Adam Tkac <atkac redhat com> 1.0.90-0.1.20091221svn3929
- update to 1.0.90 snapshot
- patches merged
  - tigervnc10-compat.patch
  - tigervnc10-rh510185.patch
  - tigervnc10-rh524340.patch
  - tigervnc10-rh516274.patch

* Mon Oct 26 2009 Adam Tkac <atkac redhat com> 1.0.0-3
- create Xvnc keyboard mapping before first keypress (#516274)

* Thu Oct 08 2009 Adam Tkac <atkac redhat com> 1.0.0-2
- update underlying X source to 1.6.4-0.3.fc11
- remove bogus '-nohttpd' parameter from /etc/sysconfig/vncservers (#525629)
- initscript LSB compliance fixes (#523974)
- improve -LowColorSwitch documentation and handling (#510185)
- honor dotWhenNoCursor option (and it's changes) every time (#524340)

* Fri Aug 28 2009 Adam Tkac <atkac redhat com> 1.0.0-1
- update to 1.0.0
- tigervnc10-rh495457.patch merged to upstream

* Mon Aug 24 2009 Karsten Hopp <karsten@redhat.com> 0.0.91-0.17
- fix ifnarch s390x for server-module

* Fri Aug 21 2009 Tomas Mraz <tmraz@redhat.com> - 0.0.91-0.16
- rebuilt with new openssl

* Tue Aug 04 2009 Adam Tkac <atkac redhat com> 0.0.91-0.15
- make Xvnc compilable

* Sun Jul 26 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.0.91-0.14.1
- Rebuilt for https://fedoraproject.org/wiki/Fedora_12_Mass_Rebuild

* Mon Jul 13 2009 Adam Tkac <atkac redhat com> 0.0.91-0.13.1
- don't write warning when initscript is called with condrestart param (#508367)

* Tue Jun 23 2009 Adam Tkac <atkac redhat com> 0.0.91-0.13
- temporary use F11 Xserver base to make Xvnc compilable
- BuildRequires: libXi-devel
- don't ship tigervnc-server-module on s390/s390x

* Mon Jun 22 2009 Adam Tkac <atkac redhat com> 0.0.91-0.12
- fix local rendering of cursor (#495457)

* Thu Jun 18 2009 Adam Tkac <atkac redhat com> 0.0.91-0.11
- update to 0.0.91 (1.0.0 RC1)
- patches merged
  - tigervnc10-rh499401.patch
  - tigervnc10-rh497592.patch
  - tigervnc10-rh501832.patch
- after discusion in upstream drop tigervnc-bounds.patch
- configure flags cleanup

* Thu May 21 2009 Adam Tkac <atkac redhat com> 0.0.90-0.10
- rebuild against 1.6.1.901 X server (#497835)
- disable i18n, vncviewer is not UTF-8 compatible (#501832)

* Mon May 18 2009 Adam Tkac <atkac redhat com> 0.0.90-0.9
- fix vncpasswd crash on long passwords (#499401)
- start session dbus daemon correctly (#497592)

* Mon May 11 2009 Adam Tkac <atkac redhat com> 0.0.90-0.8.1
- remove merged tigervnc-manminor.patch

* Tue May 05 2009 Adam Tkac <atkac redhat com> 0.0.90-0.8
- update to 0.0.90

* Thu Apr 30 2009 Adam Tkac <atkac redhat com> 0.0.90-0.7.20090427svn3789
- server package now requires xorg-x11-fonts-misc (#498184)

* Mon Apr 27 2009 Adam Tkac <atkac redhat com> 0.0.90-0.6.20090427svn3789
- update to r3789
  - tigervnc-rh494801.patch merged
- tigervnc-newfbsize.patch is no longer needed
- fix problems when vncviewer and Xvnc run on different endianess (#496653)
- UltraVNC and TightVNC clients work fine again (#496786)

* Wed Apr 08 2009 Adam Tkac <atkac redhat com> 0.0.90-0.5.20090403svn3751
- workaround broken fontpath handling in vncserver script (#494801)

* Fri Apr 03 2009 Adam Tkac <atkac redhat com> 0.0.90-0.4.20090403svn3751
- update to r3751
- patches merged
  - tigervnc-xclients.patch
  - tigervnc-clipboard.patch
  - tigervnc-rh212985.patch
- basic RandR support in Xvnc (resize of the desktop)
- use built-in libjpeg (SSE2/MMX accelerated encoding on x86 platform)
- use Tight encoding by default
- use TigerVNC icons

* Tue Mar 03 2009 Adam Tkac <atkac redhat com> 0.0.90-0.3.20090303svn3631
- update to r3631

* Tue Mar 03 2009 Adam Tkac <atkac redhat com> 0.0.90-0.2.20090302svn3621
- package review related fixes

* Mon Mar 02 2009 Adam Tkac <atkac redhat com> 0.0.90-0.1.20090302svn3621
- initial package, r3621
