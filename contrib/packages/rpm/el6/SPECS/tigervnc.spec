%{!?_bootstrap: %define _bootstrap 1}
%define tigervnc_src_dir %{_builddir}/%{name}-%{version}%{?snap:-%{snap}}
%global scl_name %{name}16
%if %{_bootstrap}
%define static_lib_buildroot %{tigervnc_src_dir}/opt/%{name}/%{scl_name}
%else
%define static_lib_buildroot /opt/%{name}/%{scl_name}
%endif

Name: tigervnc
Version: @VERSION@
Release: 7%{?snap:.%{snap}}%{?dist}
Summary: A TigerVNC remote display system

Group: User Interface/Desktops
License: GPLv2+
Packager: Brian P. Hinz <bphinz@users.sourceforge.net>
URL: http://www.tigervnc.com

Source0: %{name}-%{version}%{?snap:-%{snap}}.tar.bz2
Source1: vncserver.service
Source2: vncserver.sysconfig
Source11: http://fltk.org/pub/fltk/1.3.4/fltk-1.3.4-2-source.tar.gz
Source13: http://downloads.sourceforge.net/project/libpng/libpng16/1.6.34/libpng-1.6.34.tar.gz
Source14: https://ftp.gnu.org/gnu/gmp/gmp-6.1.2.tar.bz2
Source15: http://ftp.gnu.org/gnu/libtasn1/libtasn1-4.13.tar.gz
Source16: https://ftp.gnu.org/gnu/nettle/nettle-3.4.tar.gz
Source17: ftp://ftp.gnutls.org/gcrypt/gnutls/v3.3/gnutls-3.3.30.tar.xz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires: gcc, gcc-c++
BuildRequires: libX11-devel, automake, autoconf, libtool, gettext, gettext-devel
BuildRequires: libXext-devel, xorg-x11-server-source, libXi-devel
BuildRequires: xorg-x11-xtrans-devel, xorg-x11-util-macros, libXtst-devel
BuildRequires: libdrm-devel, libXt-devel, pixman-devel libXfont-devel
BuildRequires: libxkbfile-devel, openssl-devel, libpciaccess-devel
BuildRequires: mesa-libGL-devel, libXinerama-devel, ImageMagick
BuildRequires: freetype-devel, libXdmcp-devel, libXft-devel, libXrandr-devel
BuildRequires: libjpeg-turbo-devel, pam-devel
BuildRequires: cmake >= 2.8
%if !%{_bootstrap}
BuildRequires: %{name}-static-devel == %{version}
%endif
%ifnarch s390 s390x
BuildRequires: xorg-x11-server-devel
%endif

Requires(post): initscripts chkconfig coreutils
Requires(postun): coreutils
Requires: libjpeg-turbo
Requires: hicolor-icon-theme
Requires: tigervnc-license = %{version}-%{release}
Requires: tigervnc-icons = %{version}-%{release}

Provides: vnc = 4.1.3-2, vnc-libs = 4.1.3-2
Obsoletes: vnc < 4.1.3-2, vnc-libs < 4.1.3-2
Provides: tightvnc = 1.5.0-0.15.20090204svn3586
Obsoletes: tightvnc < 1.5.0-0.15.20090204svn3586

Patch16: tigervnc-xorg-manpages.patch

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
Requires: tigervnc-server-minimal = %{version}-%{release}
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

Requires: mesa-dri-drivers, xkeyboard-config, xorg-x11-xkb-utils
Requires: tigervnc-license = %{version}-%{release}

%description server-minimal
The VNC system allows you to access the same desktop from a wide
variety of platforms. This package contains minimal installation
of TigerVNC server, allowing others to access the desktop on your
machine.

%ifnarch s390 s390x %{?rhel:ppc ppc64}
%package server-module
Summary: TigerVNC module to Xorg
Group: User Interface/X
Provides: vnc-server = 4.1.3-2, vnc-libs = 4.1.3-2
Obsoletes: vnc-server < 4.1.3-2, vnc-libs < 4.1.3-2
Provides: tightvnc-server-module = 1.5.0-0.15.20090204svn3586
Obsoletes: tightvnc-server-module < 1.5.0-0.15.20090204svn3586
Requires: xorg-x11-server-Xorg
Requires: tigervnc-license = %{version}-%{release}

%description server-module
This package contains libvnc.so module to X server, allowing others
to access the desktop on your machine.
%endif

%package license
Summary: License of TigerVNC suite
Group: User Interface/X
BuildArch: noarch

%description license
This package contains license of the TigerVNC suite

%package icons
Summary: Icons for TigerVNC viewer
Group: User Interface/X
BuildArch: noarch

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
%setup -q -n %{name}-%{version}%{?snap:-%{snap}}

%if %{_bootstrap}
tar xzf %SOURCE11
tar xzf %SOURCE13
tar xjf %SOURCE14
tar xzf %SOURCE15
tar xzf %SOURCE16
tar xJf %SOURCE17
%endif

cp -r /usr/share/xorg-x11-server-source/* unix/xserver
pushd unix/xserver
for all in `find . -type f -perm -001`; do
	chmod -x "$all"
done
patch -p1 -b --suffix .vnc < ../xserver117.patch
popd

%patch16 -p0 -b .man

%build
%if %{_bootstrap}
mkdir -p %{static_lib_buildroot}%{_libdir}
%endif

%ifarch sparcv9 sparc64 s390 s390x
export CFLAGS="$RPM_OPT_FLAGS -fPIC -I%{static_lib_buildroot}%{_includedir}"
%else
export CFLAGS="$RPM_OPT_FLAGS -fpic -I%{static_lib_buildroot}%{_includedir}"
%endif
export CXXFLAGS=$CFLAGS
export CPPFLAGS=$CXXFLAGS
export PKG_CONFIG_PATH="%{static_lib_buildroot}%{_libdir}/pkgconfig:%{static_lib_buildroot}%{_datadir}/pkgconfig:%{_libdir}/pkgconfig:%{_datadir}/pkgconfig"

%if %{_bootstrap}
echo "*** Building gmp ***"
pushd gmp-*
./configure --prefix=%{_prefix} --libdir=%{_libdir} --enable-static --disable-shared --enable-cxx --disable-assembly
make %{?_smp_mflags} DESTDIR=%{static_lib_buildroot} install
find %{static_lib_buildroot}%{_prefix} -type f -name "*.la" -delete
find %{static_lib_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|libdir=%{_libdir}|libdir=%{static_lib_buildroot}%{_libdir}|" {} \;
find %{static_lib_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|prefix=%{_prefix}|prefix=%{static_lib_buildroot}%{_prefix}|" {} \;
popd

echo "*** Building libtasn1 ***"
pushd libtasn1-*
LDFLAGS="-L%{static_lib_buildroot}%{_libdir} $LDFLAGS" ./configure --prefix=%{_prefix} --libdir=%{_libdir} --enable-static --disable-shared --host=%{_host} --build=%{_build}
make %{?_smp_mflags} DESTDIR=%{static_lib_buildroot} install
find %{static_lib_buildroot}%{_prefix} -type f -name "*.la" -delete
find %{static_lib_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|libdir=%{_libdir}|libdir=%{static_lib_buildroot}%{_libdir}|" {} \;
find %{static_lib_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|prefix=%{_prefix}|prefix=%{static_lib_buildroot}%{_prefix}|" {} \;
popd

echo "*** Building nettle ***"
pushd nettle-*
autoreconf -fiv
LDFLAGS="-L%{static_lib_buildroot}%{_libdir} -Wl,-Bstatic -ltasn1 -lgmp -Wl,-Bdynamic $LDFLAGS" ./configure --prefix=%{_prefix} --libdir=%{_libdir} --enable-static --disable-shared --disable-openssl --host=%{_host} --build=%{_build}
make %{?_smp_mflags} DESTDIR=%{static_lib_buildroot} install
find %{static_lib_buildroot}%{_prefix} -type f -name "*.la" -delete
find %{static_lib_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|libdir=%{_libdir}|libdir=%{static_lib_buildroot}%{_libdir}|" {} \;
find %{static_lib_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|prefix=%{_prefix}|prefix=%{static_lib_buildroot}%{_prefix}|" {} \;
popd

echo "*** Building gnutls ***"
pushd gnutls-*
LDFLAGS="-L%{static_lib_buildroot}%{_libdir} -Wl,-Bstatic -lnettle -lhogweed -ltasn1 -lgmp -Wl,-Bdynamic $LDFLAGS" ./configure \
  --prefix=%{_prefix} \
  --libdir=%{_libdir} \
  --host=%{_host} \
  --build=%{_build} \
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
make %{?_smp_mflags} DESTDIR=%{static_lib_buildroot} install
find %{static_lib_buildroot}%{_prefix} -type f -name "*.la" -delete
find %{static_lib_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|libdir=%{_libdir}|libdir=%{static_lib_buildroot}%{_libdir}|" {} \;
find %{static_lib_buildroot}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|prefix=%{_prefix}|prefix=%{static_lib_buildroot}%{_prefix}|" {} \;
popd

echo "*** Building libpng ***"
pushd libpng-*
CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" LDFLAGS="${LDFLAGS}" ./configure \
  --prefix=%{_prefix} \
  --libdir=%{_libdir} \
  --host=%{_host} \
  --build=%{_build} \
  --disable-shared \
  --enable-static
make %{?_smp_mflags}
make DESTDIR=%{static_lib_buildroot} install
popd

echo "*** Building fltk ***"
pushd fltk-*
%endif
export CMAKE_PREFIX_PATH="%{static_lib_buildroot}%{_prefix}:%{_prefix}"
export CMAKE_EXE_LINKER_FLAGS=$LDFLAGS
export PKG_CONFIG="pkg-config --static"
%if %{_bootstrap}
CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" LDFLAGS="-L%{static_lib_buildroot}%{_libdir} -Wl,-Bstatic -lpng -Wl,-Bdynamic $LDFLAGS" ./configure \
  --prefix=%{_prefix} \
  --libdir=%{_libdir} \
  --host=%{_host} \
  --build=%{_build} \
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
make DESTDIR=%{static_lib_buildroot} install
popd
%endif

%{cmake} -G"Unix Makefiles" \
  -DBUILD_STATIC=off \
  -DCMAKE_INSTALL_PREFIX=%{_prefix} \
  -DFLTK_LIBRARIES="%{static_lib_buildroot}%{_libdir}/libfltk.a;%{static_lib_buildroot}%{_libdir}/libfltk_images.a;%{static_lib_buildroot}%{_libdir}/libpng.a" \
  -DFLTK_INCLUDE_DIR=%{static_lib_buildroot}%{_includedir} \
  -DGNUTLS_INCLUDE_DIR=%{static_lib_buildroot}%{_includedir} \
  -DGNUTLS_LIBRARY="%{static_lib_buildroot}%{_libdir}/libgnutls.a;%{static_lib_buildroot}%{_libdir}/libtasn1.a;%{static_lib_buildroot}%{_libdir}/libnettle.a;%{static_lib_buildroot}%{_libdir}/libhogweed.a;%{static_lib_buildroot}%{_libdir}/libgmp.a"
make %{?_smp_mflags}

pushd unix/xserver
autoreconf -fiv
%configure \
	--disable-xorg --disable-xnest --disable-xvfb --disable-dmx \
	--disable-xwin --disable-xephyr --disable-kdrive --disable-wayland \
	--with-pic --disable-static --enable-xinerama \
	--with-default-font-path="catalogue:%{_sysconfdir}/X11/fontpath.d,built-ins" \
	--with-serverconfig-path=%{_libdir}/xorg \
	--with-fontrootdir=%{_datadir}/X11/fonts \
	--with-xkb-output=%{_localstatedir}/lib/xkb \
	--enable-install-libxf86config \
	--enable-glx --enable-glx-tls --disable-dri --enable-dri2 --disable-dri3 \
	--disable-config-dbus \
	--disable-config-hal \
	--disable-config-udev \
	--without-dtrace \
	--disable-unit-tests \
	--disable-docs \
	--disable-devel-docs \
	--disable-selective-werror

make %{?_smp_mflags}
popd

# Build icons
pushd media
make
popd

%install
%if %{_bootstrap}
for l in gmp libtasn1 nettle gnutls libpng fltk; do
pushd $l-*
make install DESTDIR=$RPM_BUILD_ROOT/opt/%{name}/%{scl_name}
popd
done
find %{buildroot}/opt/%{name}/%{scl_name}%{_prefix} -type f -name "*.la" -delete
find %{buildroot}/opt/%{name}/%{scl_name}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|libdir=%{_libdir}|libdir=/opt/%{name}/%{scl_name}%{_libdir}|" {} \;
find %{buildroot}/opt/%{name}/%{scl_name}%{_prefix} -type f -name "*.pc" -exec sed -i -e "s|prefix=%{_prefix}|prefix=/opt/%{name}/%{scl_name}%{_prefix}|" {} \;
%endif

make install DESTDIR=$RPM_BUILD_ROOT

pushd unix/xserver/hw/vnc
make install DESTDIR=$RPM_BUILD_ROOT
popd

mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/init.d
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/sysconfig
install -m644 %{SOURCE1} $RPM_BUILD_ROOT%{_sysconfdir}/init.d/vncserver
install -m644 %{SOURCE2} $RPM_BUILD_ROOT%{_sysconfdir}/sysconfig/vncservers

%find_lang %{name} %{name}.lang

# remove unwanted files
rm -f  $RPM_BUILD_ROOT%{_libdir}/xorg/modules/extensions/libvnc.la

%ifarch s390 s390x %{?rhel:ppc ppc64}
rm -f $RPM_BUILD_ROOT%{_libdir}/xorg/modules/extensions/libvnc.so
%endif

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

%triggerun -- tigervnc-server < 1.0.90-6
/sbin/service vncserver stop &>/dev/null || :
/sbin/chkconfig --del vncserver >/dev/null 2>&1 || :

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

%ifnarch s390 s390x %{?rhel:ppc ppc64}
%files server-module
%defattr(-,root,root,-)
%{_libdir}/xorg/modules/extensions/libvnc.so
%endif

%files license
%defattr(-,root,root,-)
%doc LICENCE.TXT

%files icons
%defattr(-,root,root,-)
%{_datadir}/icons/hicolor/*/apps/*

%if %{_bootstrap}
%files static-devel
%defattr(-,root,root,-)
/opt/%{name}/%{scl_name}%{_bindir}/*
/opt/%{name}/%{scl_name}%{_includedir}/*
/opt/%{name}/%{scl_name}%{_libdir}/*
/opt/%{name}/%{scl_name}%{_datadir}/*
%endif

%changelog
* Mon Jan 14 2019 Pierre Ossman <ossman@cendio.se> 1.9.80-7
- Add libXrandr-devel as a dependency so x0vncserver gets resize support.

* Sun Dec 09 2018 Mark Mielke <mmielke@ciena.com> 1.9.80-6
- Update package dependencies to require version alignment between packages.

* Sun Jul 22 2018 Brian P. Hinz <bphinz@users.sourceforge.net> 1.9.80-5
- Update gnutls, libtasn1, libpng, gmp, fltk to latest upstream versions.

* Mon Jun 20 2016 Brian P. Hinz <bphinz@users.sourceforge.net> 1.6.80-5
- Patch for Xorg 1.17 due to vendor bump of Xorg version

* Sat Apr 02 2016 Brian P. Hinz <bphinz@users.sourceforge.net> 1.6.80-4
- Fixed CVE-2015-8803 CVE-2015-8804 CVE-2015-8805 secp256r1 and secp384r1 bugs

* Fri Dec 11 2015 Brian P. Hinz <bphinz@users.sourceforge.net> 1.6.80-3
- Configure with --host and --build to avoid build host-specific compiler opts

* Sun Nov 29 2015 Brian P. Hinz <bphinz@users.sourceforge.net> 1.6.80-2
- Split static pre-reqs into separate package

* Thu Nov 26 2015 Brian P. Hinz <bphinz@users.sourceforge.net> 1.6.80-1
- Version bump for 1.6 release
- Update gnutls, libtasn1, libpng to latest upstream versions.

* Sat Mar 14 2015 Brian P. Hinz <bphinz@users.sourceforge.net> 1.4.80-21
- Build static libraries to meet new minimum requirements

* Sat Mar 07 2015 Brian P. Hinz <bphinz@users.sourceforge.net> 1.4.80-20
- Don't disable xinerama extension

* Thu Feb 19 2015 Brian P. Hinz <bphinz@users.sourceforge.net> 1.4.80-19
- Bumped fltk version to 1.3.3, no longer requires any patching

* Tue Nov 04 2014 Brian P. Hinz <bphinz@users.sourceforge.net> 1.3.80-18.20131128svn5139
- Bumped xserver patch to keep pace with native version

* Thu Nov 28 2013 Brian P. Hinz <bphinz@users.sourceforge.net> 1.3.80-17.20131128svn5139
- Bumped version to 1.3.80
- Cleaned up linter warnings

* Thu Jul 05 2013 Brian P. Hinz <bphinz@users.sourceforge.net> 1.3.0
- Upstream 1.3.0 release
- Conditional-ized %snap for release

* Thu Apr 04 2013 Brian P. Hinz <bphinz@users.sourceforge.net> 1.2.90-12.20130524svn5114
- Improve spec file portability

* Thu Apr 04 2013 Brian P. Hinz <bphinz@users.sourceforge.net> 1.2.80-12.20130330svn5066
- Adapted from fedora for el6

* Thu Mar 14 2013 Adam Tkac <atkac redhat com> - 1.2.80-0.10.20130314svn5065
- include /etc/X11/xorg.conf.d/10-libvnc.conf sample configuration (#712482)
- vncserver now honors specified -geometry parameter (#755947)

* Tue Mar 12 2013 Adam Tkac <atkac redhat com> - 1.2.80-0.9.20130307svn5060
- update to r5060
- split icons to separate package to avoid multilib issues

* Thu Jan 24 2013 Adam Tkac <atkac redhat com> 1.2.80-0.8.20130124svn5036
- update to r5036 (#892351)

* Wed Jan 16 2013 Adam Tkac <atkac redhat com> 1.2.80-0.7.20121126svn5015
- rebuild

* Tue Dec 04 2012 Adam Tkac <atkac redhat com> 1.2.80-0.6.20121126svn5015
- rebuild against new fltk

* Mon Nov 26 2012 Adam Tkac <atkac redhat com> 1.2.80-0.5.20121126svn5015
- update to r5015
- build with -fpic instead of -fPIC on all archs except s390/sparc

* Wed Nov  7 2012 Peter Robinson <pbrobinson@fedoraproject.org> 1.2.80-0.4.20120905svn4996
- Build with -fPIC to fix FTBFS on ARM

* Wed Oct 31 2012 Adam Jackson <ajax@redhat.com> 1.2.80-0.3.20120905svn4996
- tigervnc12-xorg113-glx.patch: Fix to only init glx on the first server
  generation

* Fri Sep 28 2012 Adam Jackson <ajax@redhat.com> 1.2.80-0.2.20120905svn4996
- tigervnc12-xorg113-glx.patch: Re-enable GLX against xserver 1.13

* Fri Aug 17 2012 Adam Tkac <atkac redhat com> 1.2.80-0.1.20120905svn4996
- update to 1.2.80
- remove deprecated patches
  - tigervnc-102434.patch
  - tigervnc-viewer-reparent.patch
  - tigervnc11-java7.patch
- patches merged
  - tigervnc11-xorg111.patch
  - tigervnc11-xorg112.patch

* Fri Aug 10 2012 Dave Airlie <airlied@redhat.com> 1.1.0-10
- fix build against newer X server

* Mon Jul 23 2012 Adam Jackson <ajax@redhat.com> 1.1.0-9
- Build with the Composite extension for feature parity with other X servers

* Sat Jul 21 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.1.0-8
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Thu Jul 19 2012 Dave Airlie <airlied@redhat.com> 1.1.0-7
- fix building against X.org 1.13

* Wed Apr 04 2012 Adam Jackson <ajax@redhat.com> 1.1.0-6
- RHEL exclusion for -server-module on ppc* too

* Mon Mar 26 2012 Adam Tkac <atkac redhat com> - 1.1.0-5
- clean Xvnc's /tmp environment in service file before startup
- fix building against the latest JAVA 7 and X.Org 1.12

* Sat Jan 14 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.1.0-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_17_Mass_Rebuild

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
