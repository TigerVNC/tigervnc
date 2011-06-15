SUBDIRS = ../common tx x0vncserver vncviewer vncpasswd @VNCCONFIG_DIR@

dist_bin_SCRIPTS = vncserver

man1_MANS = vncserver.man

XVNCDIR = xserver/hw/vnc

EXTRA_DIST = vncserver.man $(XVNCDIR) xserver15.patch xserver16.patch \
	xserver17.patch xserver18.patch xserver19.patch build-xorg \
	download-xorg-7.4 xserver110.patch xorg-7.4-patches

dist-hook:
	rm -rf `find $(distdir) -name .svn`
