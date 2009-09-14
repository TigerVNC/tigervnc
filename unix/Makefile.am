SUBDIRS = ../common tx x0vncserver vncviewer vncpasswd @VNCCONFIG_DIR@

dist_bin_SCRIPTS = vncserver

man1_MANS = vncserver.man

XVNCDIR = xserver/hw/vnc

EXTRA_DIST = vncserver.man $(XVNCDIR)/RegionHelper.h $(XVNCDIR)/vncExtInit.cc \
	$(XVNCDIR)/buildtime.c $(XVNCDIR)/XserverDesktop.h $(XVNCDIR)/xvnc.cc \
	$(XVNCDIR)/Xvnc.man $(XVNCDIR)/Input.cc $(XVNCDIR)/xf86vncModule.cc \
	$(XVNCDIR)/XserverDesktop.cc $(XVNCDIR)/Input.h \
	$(XVNCDIR)/xorg-version.h $(XVNCDIR)/vncHooks.h \
	$(XVNCDIR)/Makefile.am $(XVNCDIR)/vncHooks.cc $(XVNCDIR)/vncExtInit.h \
	xserver15.patch xserver16.patch
