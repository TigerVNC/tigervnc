#
# spec file for package xorg-x11-Xvnc
#
# Copyright (c) 2010 SUSE LINUX Products GmbH, Nuernberg, Germany.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/
#
%define         snap 20131202svn5145
%define         debug_package %{nil}


Name:           tigervnc
Version:        1.3.80
Release:        1%{?snap:.%{snap}}%{?dist}
Packager:       Brian P. Hinz <bphinz@users.sourceforge.net>
License:        GPL-2.0 and MIT
Conflicts:      tightvnc xorg-x11-Xvnc
Requires:       xorg-x11-fonts-core xorg-x11
Requires:       libpixman-1-0 >= 0.15.2
Requires:       xinetd
BuildRequires:  Mesa-devel bison flex fontconfig-devel freetype2-devel ghostscript-library libdrm-devel libopenssl-devel pkgconfig xorg-x11 xorg-x11-devel xorg-x11-fonts-devel xorg-x11-libICE-devel xorg-x11-libSM-devel xorg-x11-libX11-devel xorg-x11-libXau-devel xorg-x11-libXdmcp-devel xorg-x11-libXext-devel xorg-x11-libXfixes-devel xorg-x11-libXmu-devel xorg-x11-libXp-devel xorg-x11-libXpm-devel xorg-x11-libXprintUtil-devel xorg-x11-libXrender-devel xorg-x11-libXt-devel xorg-x11-libXv-devel xorg-x11-libfontenc-devel xorg-x11-libxkbfile-devel xorg-x11-proto-devel xorg-x11-xtrans-devel
BuildRequires:  xorg-x11-devel gcc-c++ libstdc++-devel zlib-devel libpng-devel
%ifarch %ix86
BuildRequires:  nasm
%endif
BuildRequires:  java-devel update-alternatives
BuildRequires:  gcc make glibc-devel gettext gettext-devel intltool
%if 0%{?%suse_version} >= 1100
BuildRequires:  gettext-tools
%endif
BuildRequires:  autoconf automake libtool update-desktop-files perl
BuildRequires:  xorg-x11-server-sdk Mesa-devel libopenssl-devel gcc patch
BuildRequires:  nasm gcc-c++ pkgconfig(xproto) pkgconfig(x11) pkgconfig(xext) pkgconfig(xtst)
BuildRequires:  cmake >= 2.8 autoconf automake libtool binutils-gold
BuildRequires:  java-devel jpackage-utils
# Because of keytool to build java client
BuildRequires:  mozilla-nss
BuildRequires:  xorg-x11-libICE-devel xorg-x11-libSM-devel pkgconfig(libtasn1) pkgconfig(gnutls) libgcrypt-devel libgpg-error-devel pam-devel
BuildRequires:  pkgconfig(fontutil)
BuildRequires:  pkg-config xmlto pkgconfig(pixman-1) >= 0.15.20 pkgconfig(glproto) pkgconfig(gl) pkgconfig(dri)  pkgconfig(openssl)
BuildRequires:  pkgconfig(fixesproto) >= 4.1 pkgconfig(damageproto) >= 1.1 pkgconfig(xcmiscproto) >= 1.2.0 
BuildRequires:  pkgconfig(bigreqsproto) >= 1.1.0 pkgconfig(xproto)  >= 7.0.17 pkgconfig(randrproto) >= 1.2.99.3 pkgconfig(renderproto) >= 0.11 
BuildRequires:  pkgconfig(xextproto) >= 7.0.99.3 pkgconfig(inputproto)  >= 1.9.99.902 pkgconfig(kbproto) >= 1.0.3 pkgconfig(fontsproto)
BuildRequires:  pkgconfig(videoproto) pkgconfig(compositeproto) >= 0.4 pkgconfig(recordproto) >= 1.13.99.1 pkgconfig(scrnsaverproto) >= 1.1 
BuildRequires:  pkgconfig(resourceproto) pkgconfig(xineramaproto) pkgconfig(xkbfile) pkgconfig(xau) pkgconfig(xdmcp) pkgconfig(xfont) >= 1.4.2
BuildRequires:  pkgconfig(pciaccess) >= 0.8.0 
URL:            http://sourceforge.net/apps/mediawiki/tigervnc/
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
Group:          System/X11/Servers/XF86_4
Summary:        A high-performance, platform-neutral implementation of VNC
Source1:        %{name}-%{version}%{?snap:-%{snap}}.tar.bz2
Source2:        xorg-server-1.9.3.tar.bz2
Source3:        vnc.xinetd
Source4:        10-libvnc.conf
Source5:        vnc-server.firewall
Source6:        vnc-httpd.firewall
Source7:        vnc_inetd_httpd
Source8:        vnc.reg
Source11:       http://fltk.org/pub/fltk/1.3.2/fltk-1.3.2-source.tar.gz
Source12:       http://downloads.sourceforge.net/project/libjpeg-turbo/1.3.0/libjpeg-turbo-1.3.0.tar.gz


# Tiger vnc patches
Patch3:         u_tigervnc-1.3.0-fix-use-after-free.patch
Patch4:         tigervnc-newfbsize.patch
Patch5:         tigervnc-clean-pressed-key-on-exit.patch

# Xserver patches
Patch1:         fpic.diff
Patch2:         p_default-module-path.diff
Patch6:         pu_fixes.diff
Patch8:         p_bug96328.diff
Patch13:        p_xorg_acpi.diff
Patch14:        p_xkills_wrong_client.diff
Patch16:        p_xnest-ignore-getimage-errors.diff
Patch23:        disable-fbblt-opt.diff
Patch27:        mouse.diff
Patch29:        xephyr.diff
Patch36:        libdrm.diff

Patch45:        bug-197858_dpms.diff
Patch67:        xorg-docs.diff
Patch77:        fbdevhw.diff
Patch79:        edit_data_sanity_check.diff
Patch93:        pixman.diff
Patch101:       zap_warning_xserver.diff
Patch103:       confine_to_shape.diff
Patch104:       bitmap_always_unscaled.diff
Patch106:       randr1_1-sig11.diff
Patch112:       fix-dpi-values.diff
Patch123:       vidmode-sig11.diff
Patch125:       0001-Xinput-Catch-missing-configlayout-when-deleting-dev.patch
Patch127:       dpms_screensaver.diff
Patch128:       pci-legacy-mem-fallback.diff
Patch129:       bug474071-fix1.diff
Patch143:       autoconfig_fallback_fbdev_first.diff
Patch145:       driver-autoconfig.diff
Patch147:       xserver-1.6.1-nouveau.patch
Patch162:       cache-xkbcomp-output-for-fast-start-up.patch
Patch163:       xserver-bg-none-root.patch
Patch164:       xorg-detect-psb.patch
Patch168:       xorg-server-nohwaccess.diff
Patch169:       xorg-x11-nonroot-vesa.patch
Patch200:       bug534768-prefer_local_symbols.patch
Patch202:       0001-Check-harder-for-primary-PCI-device.patch
Patch203:       0001-Fix-segfault-when-killing-X-with-ctrl-alt-backspace.patch
Patch204:       missing_font_paths.diff
Patch205:       xorg-server-1.8.0.diff
Patch206:       fix_fglrx_screendepth_issue.patch
Patch207:       xorg-server-option_libxf86config.diff
Patch210:       pio_ia64.diff
Patch211:       0001-Prevent-XSync-Alarms-from-senslessly-calling-CheckTr.patch
Patch213:       xorg-server-xdmcp.patch
Patch217:       CVE-2010-2240-address_space_limit.patch
Patch218:       CVE-2010-2240-tree_depth_limit.patch
Patch220:       Use-external-tool-for-creating-backtraces-on-crashes.patch
Patch221:       commit-5c6a2f9.diff
Patch222:       sync-fix.patch
Patch223:       use-last-screen.patch
Patch224:       pad-size-of-system-memory-copy-for-1x1-pixmaps
Patch225:       xorg-server-stop-cpu-eating.diff
Patch226:       EXA-mixed-ModifyPixmapHeader-pitch-fixes.-bug-33929.patch
Patch227:       U_01-glx-make-sure-screen-is-non-negative-in-validGlxScreen.patch
Patch228:       U_02-glx-validate-request-lengths.patch
Patch229:       U_03-glx-check-request-length-before-swapping.patch
Patch230:       U_04-glx-swap-the-request-arrays-entirely-not-just-half-of-them.patch
Patch231:       U_05-glx-validate-numAttribs-field-before-using-it.patch
Patch232:       U_06-glx-fix-request-length-check-for-CreateGLXPbufferSGIX.patch
Patch233:       U_07-glx-fix-BindTexImageEXT-length-check.patch
Patch234:       U_08-glx-Work-around-wrong-request-lengths-sent-by-mesa.patch
Patch235:       U_xf86-fix-flush-input-to-work-with-Linux-evdev-device.patch
Patch236:       u_xserver_xvfb-randr.patch
Patch237:       U_os-Reset-input-buffer-s-ignoreBytes-field.patch
Patch238:       u_Avoid-use-after-free-in-dix-dixfonts.c-doImageText.patch

# Fltk patches
# Export dead key information from FLTK to the apps
# http://www.fltk.org/str.php?L2599
Patch510: http://www.fltk.org/strfiles/2599/fltk-1_v4.3.x-keyboard-x11.patch
Patch511: http://www.fltk.org/strfiles/2599/fltk-1_v4.3.x-keyboard-win32.patch
Patch512: http://www.fltk.org/strfiles/2599/fltk-1_v6.3.x-keyboard-osx.patch

# Notify applications of changes to the clipboard
# http://www.fltk.org/str.php?L2636
Patch513: http://www.fltk.org/strfiles/2636/fltk-1.3.x-clipboard.patch
Patch514: http://www.fltk.org/strfiles/2636/fltk-1_v6.3.x-clipboard-x11.patch
Patch515: http://www.fltk.org/strfiles/2636/fltk-1_v3.3.x-clipboard-win32-fix.patch
Patch516: http://www.fltk.org/strfiles/2636/fltk-1_v2.3.x-clipboard-win32.patch
Patch517: http://www.fltk.org/strfiles/2636/fltk-1_v2.3.x-clipboard-osx.patch

# Ability to convert a Fl_Pixmap to a Fl_RGB_Image
# http://www.fltk.org/str.php?L2659
Patch518: http://www.fltk.org/strfiles/2659/pixmap_v2.patch

# Support for custom cursors
# http://www.fltk.org/str.php?L2660
Patch519: http://www.fltk.org/strfiles/2660/fltk-1_v5.3.x-cursor.patch

# Improve modality interaction with WM
# http://www.fltk.org/str.php?L2802
Patch520: http://www.fltk.org/strfiles/2802/fltk-1_v2.3.0-modal.patch

# Window icons
# http://www.fltk.org/str.php?L2816
Patch521: http://www.fltk.org/strfiles/2816/fltk-1_v3.3.0-icons.patch

# Multihead
# http://fltk.org/str.php?L2860
Patch522: http://www.fltk.org/strfiles/2860/fltk-1.3.x-screen_num.patch
Patch523: http://www.fltk.org/strfiles/2860/fltk-1_v3.3.x-multihead.patch

%description
TigerVNC is a high-performance, platform-neutral implementation of VNC (Virtual Network Computing), 
a client/server application that allows users to launch and interact with graphical applications on remote machines. 
TigerVNC provides the levels of performance necessary to run 3D and video applications;
it attempts to maintain a common look and feel and re-use components, where possible, across the various platforms that it supports. 
TigerVNC also provides extensions for advanced authentication methods and TLS encryption.

%prep
%setup -T -b1 -q -n %{name}-%{version}%{?snap:-%{snap}}

tar xjf %SOURCE2
cp -r ./xorg-server-*/* unix/xserver/

%patch3 -p1
%patch4 -p1
%patch5 -p1

pushd unix/xserver
for all in `find . -type f -perm -001`; do
  chmod -x "$all"
done
patch -p1 < ../xserver19.patch
%patch1
%patch2
%patch6
%patch8 -p0
%patch13
%patch14
%patch16 -p2
%patch23
%patch27
%patch29
%patch36 -p0

%patch45 -p0
%patch77
%patch79 -p1
%patch93
%patch101 -p1
%patch103
%patch104 -p1
%patch106 -p1
%patch112 -p0
%patch123 -p0
%patch125 -p1
%patch127 -p1
%patch128
pushd hw/xfree86/os-support/bus
%patch129 -p0
popd
%patch143 -p0
%patch145 -p0
%patch147 -p1
%patch162 -p1
%patch163 -p1
%patch164 -p1
%patch168 -p1
%patch169 -p1
%patch200 -p1
%patch202 -p1
%patch203 -p1
%patch204 -p0
%patch205 -p0
%patch206 -p0
%patch207 -p0
%patch210 -p1
%patch211 -p1
%patch213 -p1
%patch217 -p1
%patch218 -p1

%patch221 -p1
%patch222 -p1
%patch223 -p1
%patch224 -p1
%patch225 -p1
%patch226 -p1
%patch227 -p1
%patch228 -p1
%patch229 -p1
%patch230 -p1
%patch231 -p1
%patch232 -p1
%patch233 -p1
%patch234 -p1
%patch235 -p1
%patch236 -p1
%patch237 -p1
%patch238 -p1

popd

tar xzf %SOURCE11
pushd fltk-*
%patch510 -p1 -b .keyboard-x11
%patch511 -p1 -b .keyboard-win32
%patch512 -p1 -b .keyboard-osx
%patch513 -p1 -b .clipboard
%patch514 -p1 -b .clipboard-x11
%patch515 -p1 -b .clipboard-win32-fix
%patch516 -p1 -b .clipboard-win32
%patch517 -p1 -b .clipboard-osx
%patch518 -p1 -b .pixmap
%patch519 -p1 -b .cursor
%patch520 -p1 -b .modal
%patch521 -p1 -b .icons
%patch522 -p1 -b .screen_num
%patch523 -p1 -b .multihead
popd

tar xzf %SOURCE12

%build
%define tigervnc_src_dir %{_builddir}/%{name}-%{version}%{?snap:-%{snap}}
%define static_lib_buildroot %{tigervnc_src_dir}/build
%ifarch sparcv9 sparc64 s390 s390x
export CFLAGS="$RPM_OPT_FLAGS -fPIC"
%else
export CFLAGS="$RPM_OPT_FLAGS -fpic"
%endif
export CXXFLAGS="$CFLAGS"

echo "*** Building fltk ***"
pushd fltk-*
cmake -G"Unix Makefiles" \
  -DCMAKE_INSTALL_PREFIX=%{_prefix} \
  -DCMAKE_BUILD_TYPE=Release \
  -DOPTION_PREFIX_LIB=%{_libdir} \
  -DOPTION_PREFIX_CONFIG=%{_libdir} \
  -DOPTION_BUILD_EXAMPLES=off \
  -DOPTION_USE_SYSTEM_LIBPNG=on
make %{?_smp_mflags}
popd

echo "*** Building libjpeg-turbo ***"
pushd libjpeg-turbo-*
export CXXFLAGS="$CFLAGS -static-libgcc"
./configure --prefix=%{_prefix} --libdir=%{_libdir} --disable-nls --enable-static --disable-shared
make %{?_smp_mflags} DESTDIR=%{static_lib_buildroot} install
popd

# Build all tigervnc
cmake -G"Unix Makefiles" \
  -DCMAKE_INSTALL_PREFIX:PATH=%{_prefix} \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DJPEG_INCLUDE_DIR=%{static_lib_buildroot}%{_includedir} \
  -DJPEG_LIBRARY=%{static_lib_buildroot}%{_libdir}/libjpeg.a \
  -DFLTK_LIBRARIES="%{tigervnc_src_dir}/fltk-1.3.2/lib/libfltk.a;%{tigervnc_src_dir}/fltk-1.3.2/lib/libfltk_images.a;-lpng" \
  -DFLTK_INCLUDE_DIR=%{tigervnc_src_dir}/fltk-1.3.2
make LDFLAGS="-lpng" %{?_smp_mflags}
make %{?_smp_mflags}

# Build Xvnc server
pushd unix/xserver
autoreconf -fiv
chmod +x ./configure
%configure CFLAGS="$RPM_OPT_FLAGS -fno-strict-aliasing" \
        --sysconfdir=/etc \
        --enable-xdmcp \
        --enable-xdm-auth-1 \
        --enable-record \
        --enable-xcsecurity \
        --with-sha1=libcrypto \
        --disable-xorg --disable-xnest --disable-xvfb --disable-dmx \
        --disable-xwin --disable-xephyr --disable-kdrive --with-pic \
        --disable-static --disable-xinerama \
        --with-log-dir="/var/log" \
        --with-os-name="openSUSE" \
        --with-os-vendor="SUSE LINUX" \
        --with-xkb-path="/usr/share/X11/xkb" \
        --with-xkb-output="/var/lib/xkb/compiled" \
        --enable-glx --disable-dri --enable-dri2 \
        --disable-config-dbus \
        --disable-config-hal \
        --disable-config-udev \
        --without-dtrace \
        --disable-unit-tests \
        --disable-devel-docs \
        --with-fontrootdir=/usr/share/fonts \
        --disable-selective-werror
make %{?_smp_mflags}
popd

# Build icons
pushd media
make
popd

# Build java client
pushd java
cmake -DCMAKE_INSTALL_PREFIX:PATH=%{_prefix}
make %{?_smp_mflags}
popd

%install

%make_install

pushd unix/xserver
%make_install
popd

pushd java
mkdir -p $RPM_BUILD_ROOT%{_datadir}/vnc/classes
install -m755 VncViewer.jar $RPM_BUILD_ROOT%{_datadir}/vnc/classes
install -m644 com/tigervnc/vncviewer/index.vnc $RPM_BUILD_ROOT%{_datadir}/vnc/classes
popd

# Install desktop stuff
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/{16x16,24x24,48x48}/apps

pushd media/icons
for s in 16 24 48; do
install -m644 tigervnc_$s.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/${s}x$s/apps/tigervnc.png
done
popd

install -D -m 644 %{SOURCE3} $RPM_BUILD_ROOT/etc/xinetd.d/vnc
install -D -m 644 %{SOURCE4} $RPM_BUILD_ROOT/etc/X11/xorg.conf.d/10-libvnc.conf
install -D -m 644 %{SOURCE5} $RPM_BUILD_ROOT/etc/sysconfig/SuSEfirewall2.d/services/vnc-server
install -D -m 644 %{SOURCE6} $RPM_BUILD_ROOT/etc/sysconfig/SuSEfirewall2.d/services/vnc-httpd
install -D -m 755 %{SOURCE7} $RPM_BUILD_ROOT%{_bindir}/vnc_inetd_httpd
install -D -m 644 %{SOURCE8} $RPM_BUILD_ROOT/etc/slp.reg.d/vnc.reg

%find_lang '%{name}'

%files -f %{name}.lang
%defattr(-,root,root,-)
%{_bindir}/vncviewer
%exclude /usr/share/doc/tigervnc-%{version}
%doc LICENCE.TXT
%doc README.txt
%{_mandir}/man1/vncviewer.1*

%{_bindir}/Xvnc
%{_bindir}/vncconfig
%{_bindir}/vncpasswd
%{_bindir}/vncserver
%{_bindir}/x0vncserver
%{_bindir}/vnc_inetd_httpd

%exclude %{_mandir}/man1/Xserver.1*
%{_mandir}/man1/Xvnc.1*
%{_mandir}/man1/vncconfig.1*
%{_mandir}/man1/vncpasswd.1*
%{_mandir}/man1/vncserver.1*
%{_mandir}/man1/x0vncserver.1*

%exclude /usr/%{_lib}/xorg/protocol.txt
%exclude /usr/%{_lib}/xorg/modules/extensions/libvnc.la
%{_libdir}/xorg/modules/extensions/libvnc.so

%exclude /var/lib/xkb/compiled/README.compiled

/etc/sysconfig/SuSEfirewall2.d/services/vnc-server
/etc/sysconfig/SuSEfirewall2.d/services/vnc-httpd

%config(noreplace) /etc/X11/xorg.conf.d/10-libvnc.conf
%config(noreplace) /etc/xinetd.d/vnc
%dir /etc/slp.reg.d
%config(noreplace) /etc/slp.reg.d/vnc.reg

%doc java/com/tigervnc/vncviewer/README
%{_datadir}/vnc

%{_datadir}/icons/hicolor/*/apps/*

%changelog
* Sat Dec  7 2013 Brian P. Hinz <bphinz@users.sourceforge.net>
- Initial spec file created based on existing SLES & OpenSUSE spec files
