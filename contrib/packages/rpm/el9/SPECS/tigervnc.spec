#defining macros needed by SELinux
%global selinuxtype targeted
%global modulename vncsession

Name:           tigervnc
Version:        @VERSION@
Release:        1%{?snap:.%{snap}}%{?dist}
Summary:        A TigerVNC remote display system

%global _hardened_build 1

License:        GPLv2+
URL:            http://www.tigervnc.com

Source0:        %{name}-%{version}%{?snap:-%{snap}}.tar.bz2
Source3:        10-libvnc.conf

BuildRequires:  make
BuildRequires:  gcc-c++
BuildRequires:  automake, autoconf, libtool, gettext, gettext-autopoint
BuildRequires:  cmake, desktop-file-utils, appstream
BuildRequires:  libxkbfile-devel, openssl-devel, libpciaccess-devel
BuildRequires:  freetype-devel, libjpeg-turbo-devel, pam-devel
BuildRequires:  gnutls-devel, nettle-devel, gmp-devel
BuildRequires:  zlib-devel
# X11/graphics dependencies
BuildRequires: xorg-x11-server-source
BuildRequires: libXext-devel, libX11-devel, libXi-devel, libXfixes-devel
BuildRequires: libXdamage-devel, libXrandr-devel, libXt-devel, libXdmcp-devel
BuildRequires: libXinerama-devel, mesa-libGL-devel, libxshmfence-devel
BuildRequires: pixman-devel, libdrm-devel,
BuildRequires: xorg-x11-util-macros, xorg-x11-xtrans-devel, libXtst-devel
BuildRequires: xorg-x11-font-utils
BuildRequires:  libXfont2-devel
# SELinux
BuildRequires:  libselinux-devel, selinux-policy-devel, systemd

# TigerVNC 1.4.x requires fltk 1.3.3 for keyboard handling support
# See https://github.com/TigerVNC/tigervnc/issues/8, also bug #1208814
BuildRequires:  fltk-devel >= 1.3.3
BuildRequires:  xorg-x11-server-devel

Requires:       hicolor-icon-theme
Requires:       tigervnc-license
Requires:       tigervnc-icons

%description
Virtual Network Computing (VNC) is a remote display system which
allows you to view a computing 'desktop' environment not only on the
machine where it is running, but from anywhere on the Internet and
from a wide variety of machine architectures.  This package contains a
client which will allow you to connect to other desktops running a VNC
server.

%package server
Summary:        A TigerVNC server
Requires:       perl-interpreter
Requires:       tigervnc-server-minimal = %{version}-%{release}
Requires:       (%{name}-selinux if selinux-policy-%{selinuxtype})
Requires:       xorg-x11-xauth
Requires:       xorg-x11-xinit

%description server
The VNC system allows you to access the same desktop from a wide
variety of platforms.  This package includes set of utilities
which make usage of TigerVNC server more user friendly. It also
contains x0vncserver program which can export your active
X session.

%package server-minimal
Summary:        A minimal installation of TigerVNC server
Requires(post): systemd
Requires(preun): systemd
Requires(postun): systemd
Requires(post): systemd

Requires:       mesa-dri-drivers, xkeyboard-config, xkbcomp
Requires:       tigervnc-license, dbus-x11

%description server-minimal
The VNC system allows you to access the same desktop from a wide
variety of platforms. This package contains minimal installation
of TigerVNC server, allowing others to access the desktop on your
machine.

%package server-module
Summary:        TigerVNC module to Xorg
Requires:       xorg-x11-server-Xorg %(xserver-sdk-abi-requires ansic) %(xserver-sdk-abi-requires videodrv)
Requires:       tigervnc-license

%description server-module
This package contains libvnc.so module to X server, allowing others
to access the desktop on your machine.

%package license
Summary:        License of TigerVNC suite
BuildArch:      noarch

%description license
This package contains license of the TigerVNC suite

%package icons
Summary:        Icons for TigerVNC viewer
BuildArch:      noarch

%description icons
This package contains icons for TigerVNC viewer

%package selinux
Summary:        SELinux module for TigerVNC
BuildArch:      noarch
BuildRequires:  selinux-policy-devel
Requires:       selinux-policy-%{selinuxtype}
Requires(post): selinux-policy-%{selinuxtype}
BuildRequires:  selinux-policy-devel
%{?selinux_requires}

%description selinux
This package provides the SELinux policy module to ensure TigerVNC
runs properly under an environment with SELinux enabled.

%prep
%setup -q -n %{name}-%{version}%{?snap:-%{snap}}

cp -r /usr/share/xorg-x11-server-source/* unix/xserver
pushd unix/xserver
for all in `find . -type f -perm -001`; do
        chmod -x "$all"
done
xserver_patch="../xserver$(rpm -q --qf '%%{VERSION}' xorg-x11-server-source | awk -F. '{ print $1 $2 }').patch"
patch -p1 -b --suffix .vnc < "$xserver_patch"
popd

%build
%ifarch sparcv9 sparc64 s390 s390x
export CFLAGS="$RPM_OPT_FLAGS -fPIC"
%else
export CFLAGS="$RPM_OPT_FLAGS -fpic"
%endif
export CXXFLAGS="$CFLAGS -std=c++11"

%cmake

%cmake_build

pushd unix/xserver

autoreconf -fiv
%configure \
        --disable-xorg --disable-xnest --disable-xvfb --disable-dmx \
        --disable-xwin --disable-xephyr --disable-kdrive --disable-xwayland \
        --with-pic --disable-static \
        --with-default-font-path="catalogue:%{_sysconfdir}/X11/fontpath.d,built-ins" \
        --with-fontdir=%{_datadir}/X11/fonts \
        --with-xkb-output=%{_localstatedir}/lib/xkb \
        --enable-install-libxf86config \
        --enable-glx --disable-dri --enable-dri2 --disable-dri3 \
        --disable-unit-tests \
        --disable-config-hal \
        --disable-config-udev \
        --with-dri-driver-path=%{_libdir}/dri \
        --without-dtrace \
        --disable-devel-docs \
        --disable-selective-werror

make TIGERVNC_BUILDDIR="`pwd`/../../%{__cmake_builddir}" %{?_smp_mflags}
popd

# SELinux
pushd unix/vncserver/selinux
make
popd

%install
%cmake_install

pushd unix/xserver/hw/vnc
%make_install TIGERVNC_BUILDDIR="`pwd`/../../../../%{__cmake_builddir}"
popd

# Install systemd unit file
pushd unix/vncserver/selinux
make install DESTDIR=%{buildroot}
popd

%find_lang %{name} %{name}.lang

# remove unwanted files
rm -f  %{buildroot}%{_libdir}/xorg/modules/extensions/libvnc.la

mkdir -p %{buildroot}%{_sysconfdir}/X11/xorg.conf.d/
install -m 644 %{SOURCE3} %{buildroot}%{_sysconfdir}/X11/xorg.conf.d/10-libvnc.conf

%pre selinux
%selinux_relabel_pre -s %{selinuxtype}

%post selinux
%selinux_modules_install -s %{selinuxtype} %{_datadir}/selinux/packages/%{selinuxtype}/%{modulename}.pp.bz2
%selinux_relabel_post -s %{selinuxtype}

%postun selinux
if [ $1 -eq 0 ]; then
    %selinux_modules_uninstall -s %{selinuxtype} %{modulename}
    %selinux_relabel_post -s %{selinuxtype}
fi


%files -f %{name}.lang
%doc %{_docdir}/%{name}/README.rst
%{_bindir}/vncviewer
%{_datadir}/applications/*
%{_datadir}/metainfo/*
%{_mandir}/man1/vncviewer.1*

%files server
%config(noreplace) %{_sysconfdir}/pam.d/tigervnc
%config(noreplace) %{_sysconfdir}/tigervnc/vncserver-config-defaults
%config(noreplace) %{_sysconfdir}/tigervnc/vncserver-config-mandatory
%config(noreplace) %{_sysconfdir}/tigervnc/vncserver.users
%{_unitdir}/vncserver@.service
%{_bindir}/x0vncserver
%{_sbindir}/vncsession
%{_libexecdir}/vncserver
%{_libexecdir}/vncsession-start
%{_mandir}/man1/x0vncserver.1*
%{_mandir}/man8/vncserver.8*
%{_mandir}/man8/vncsession.8*
%doc %{_docdir}/%{name}/HOWTO.md

%files server-minimal
%{_bindir}/vncconfig
%{_bindir}/vncpasswd
%{_bindir}/Xvnc
%{_mandir}/man1/Xvnc.1*
%{_mandir}/man1/vncpasswd.1*
%{_mandir}/man1/vncconfig.1*

%files server-module
%{_libdir}/xorg/modules/extensions/libvnc.so
%config(noreplace) %{_sysconfdir}/X11/xorg.conf.d/10-libvnc.conf

%files license
%doc %{_docdir}/%{name}/LICENCE.TXT

%files icons
%{_datadir}/icons/hicolor/*/apps/*

%files selinux
%{_datadir}/selinux/packages/%{selinuxtype}/%{modulename}.pp.*
%ghost %verify(not md5 size mtime) %{_sharedstatedir}/selinux/%{selinuxtype}/active/modules/200/%{modulename}

%changelog
* Fri Aug 19 2022 Pierre Ossman <ossman@cendio.se> 1.12.80-1
- Synced with current Fedora packaging

* Tue May 18 2021 Jan Grulich <jgrulich@redhat.com> 1.11.0-1
- SELinux package improvements

* Mon Jul 27 2020 Mark Mielke <mmielke@ciena.com> 1.10.1-1
- Update build requirements and fix unexpected rpm macro expansion.

* Tue Oct 22 2019 Evan Burns <evanburnsdev@gmail.com> 1.9.80-5
- Add support for CentOS 8

* Mon Feb 11 2019 Mark Mielke <mmielke@ciena.com> 1.9.80-5
- Automatically detect and apply the correct X.org patch.

* Mon Jan 14 2019 Pierre Ossman <ossman@cendio.se> 1.9.80-4
- Use system FLTK for build
- Add libXrandr-devel as a dependency so x0vncserver gets resize support.

* Sun Dec 09 2018 Mark Mielke <mmielke@ciena.com> 1.9.80-3
- Update package dependencies to require version alignment between packages.

* Mon Nov 26 2018 Brian P. Hinz <bphinz@users.sourceforge.net> 1.9.80-2
- Bumped Xorg version to 1.20

* Sun Jul 22 2018 Brian P. Hinz <bphinz@users.sourceforge.net> 1.9.80-1
- Updated fltk to latest version

* Thu Dec 24 2015 Brian P. Hinz <bphinz@users.sourceforge.net> 1.6.80-1
- Adapted from RedHat EL7 Spec

* Wed Sep 02 2015 Jan Grulich <jgrulich@redhat.com> - 1.3.1-3
- Do not mention that display number is required in the file name
  Resolves: bz#1195266

* Thu Jul 30 2015 Jan Grulich <jgrulich@redhat.com> - 1.3.1-2
- Resolves: bz#1248422
  CVE-2014-8240 CVE-2014-8241 tigervnc: various flaws

* Wed Apr 15 2015 Jan Grulich <jgrulich@redhat.com> - 1.3.1-1
- Drop unecessary patches
- Re-base to 1.3.1 (bug #1199453)
- Re-build against re-based xserver (bug #1194898)
- Check the return value from XShmAttach (bug #1072733)
- Add missing part of xserver114.patch (bug #1140603)
- Keep pointer in sync (bug #1100661)
- Make input device class global (bug #1119640)
- Add IPv6 support (bug #1162722)
- Set initial mode as prefered (bug #1181287)
- Do not mention that display number is required in the file name (bug #1195266)
- Enable Xinerama extension (bug #1199437)
- Specify full path for runuser command (bug #1208817)

* Tue Sep 23 2014 Tim Waugh <twaugh@redhat.com> - 1.2.80-0.31.20130314svn5065
- Rebuilt against xorg-x11-server to pick up ppc64le fix (bug #1140424).

* Mon Mar 10 2014 Tim Waugh <twaugh@redhat.com> - 1.2.80-0.30.20130314svn5065
- Fixed heap-based buffer overflow (CVE-2014-0011, bug #1050928).

* Tue Feb 18 2014 Tim Waugh <twaugh@redhat.com> - 1.2.80-0.29.20130314svn5065
- Previous patch was not applied.

* Mon Feb 10 2014 Tim Waugh <twaugh@redhat.com> - 1.2.80-0.28.20130314svn5065
- Clearer xstartup file (bug #923655).

* Tue Jan 28 2014 Tim Waugh <twaugh@redhat.com> - 1.2.80-0.27.20130314svn5065
- Use keyboard input code from tigervnc-1.3.0 (bug #1053536).

* Fri Jan 24 2014 Daniel Mach <dmach@redhat.com> - 1.2.80-0.26.20130314svn5065
- Mass rebuild 2014-01-24

* Fri Jan 10 2014 Tim Waugh <twaugh@redhat.com> - 1.2.80-0.25.20130314svn5065
- Fixed viewer crash when cursor has not been set (bug #1051333).

* Fri Dec 27 2013 Daniel Mach <dmach@redhat.com> - 1.2.80-0.24.20130314svn5065
- Mass rebuild 2013-12-27

* Thu Dec 12 2013 Tim Waugh <twaugh@redhat.com> 1.2.80-0.23.20130314svn5065
- Avoid invalid read when ZRLE connection closed (bug #1039926).

* Tue Dec 10 2013 Tim Waugh <twaugh@redhat.com> 1.2.80-0.22.20130314svn5065
- Fixed GLX initialisation (bug #1039126).

* Tue Nov 19 2013 Tim Waugh <twaugh@redhat.com> 1.2.80-0.21.20130314svn5065
- Better fix for PIDFile problem (bug #1031625).

* Fri Nov 08 2013 Adam Jackson <ajax@redhat.com> 1.2.80-0.20.20130314svn5065
- Rebuild against xserver 1.15RC1

* Wed Jul 24 2013 Tim Waugh <twaugh@redhat.com> 1.2.80-0.18.20130314svn5065
- Avoid PIDFile problems in systemd unit file (bug #983232).
- Don't use shebang in vncserver script.

* Wed Jul  3 2013 Tim Waugh <twaugh@redhat.com> 1.2.80-0.18.20130314svn5065
- Removed systemd_requires macro in order to fix the build.

* Wed Jul  3 2013 Tim Waugh <twaugh@redhat.com> 1.2.80-0.17.20130314svn5065
- Synchronise manpages and --help output (bug #980870).

* Mon Jun 17 2013 Adam Jackson <ajax@redhat.com> 1.2.80-0.16.20130314svn5065
- tigervnc-setcursor-crash.patch: Attempt to paper over a crash in Xvnc when
  setting the cursor.

* Sat Jun 08 2013 Dennis Gilmore <dennis@ausil.us> 1.2.80-0.15.20130314svn5065
- bump to rebuild and pick up bugfix causing X to crash on ppc and arm

* Thu May 23 2013 Tim Waugh <twaugh@redhat.com> 1.2.80-0.14.20130314svn5065
- Use systemd rpm macros (bug #850340).  Moved systemd requirements
  from main package to server sub-package.
- Applied Debian patch to fix busy loop when run from inetd in nowait
  mode (bug #920373).
- Added dependency on xorg-x11-xinit to server sub-package so that
  default window manager can be found (bug #896284, bug #923655).
- Fixed bogus changelog date.

* Thu Mar 14 2013 Adam Jackson <ajax@redhat.com> 1.2.80-0.13.20130314svn5065
- Less RHEL customization

* Thu Mar 14 2013 Adam Tkac <atkac redhat com> - 1.2.80-0.12.20130314svn5065
- include /etc/X11/xorg.conf.d/10-libvnc.conf sample configuration (#712482)
- vncserver now honors specified -geometry parameter (#755947)

* Tue Mar 12 2013 Adam Tkac <atkac redhat com> - 1.2.80-0.11.20130307svn5060
- update to r5060
- split icons to separate package to avoid multilib issues

* Tue Feb 19 2013 Adam Tkac <atkac redhat com> - 1.2.80-0.10.20130219svn5047
- update to r5047 (X.Org 1.14 support)

* Fri Feb 15 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.2.80-0.9.20121126svn5015
- Rebuilt for https://fedoraproject.org/wiki/Fedora_19_Mass_Rebuild

* Mon Jan 21 2013 Adam Tkac <atkac redhat com> - 1.2.80-0.8.20121126svn5015
- rebuild due to "jpeg8-ABI" feature drop

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

* Thu May 12 2011 Adam Tkac <atkac redhat com> - 1.0.90-5
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
