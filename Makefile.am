SUBDIRS = common po

if BUILD_WIN
SUBDIRS += win
else
SUBDIRS += unix
endif

ACLOCAL_AMFLAGS = -I m4

if X86_64

udmg: all
	sh makemacpkg universal ${BUILDDIR32}

endif

dmg: all
	sh makemacpkg

tarball: all
	sh maketarball

servertarball: all
	sh maketarball server

EXTRA_DIST = CMakeLists.txt cmakescripts/getdate.bat LICENCE.TXT \
	doc java media release config.h.cmake.in m4 BUILDING.txt

dist-hook:
	rm -rf `find $(distdir) -name .svn`

java-viewer:
	cd $(srcdir)/java/src/com/tigervnc/vncviewer; \
	$(MAKE)

xserver-install: install java-viewer
	$(INSTALL) -m 755 ./xorg.build/bin/Xvnc $(DESTDIR)/$(bindir)/
	$(INSTALL) -m 644 ./xorg.build/man/man1/Xvnc.1 $(DESTDIR)/$(mandir)/man1/Xvnc.1
	$(INSTALL) -m 644 ./xorg.build/man/man1/Xserver.1 $(DESTDIR)/$(mandir)/man1/Xserver.1
	mkdir -p $(DESTDIR)/$(prefix)/vnc/classes/
	$(INSTALL) -m 644 $(srcdir)/java/src/com/tigervnc/vncviewer/index.vnc $(DESTDIR)/$(prefix)/vnc/classes/
	$(INSTALL) -m 644 $(srcdir)/java/src/com/tigervnc/vncviewer/VncViewer.jar $(DESTDIR)/$(prefix)/vnc/classes/
	$(INSTALL) -m 644 $(srcdir)/java/src/com/tigervnc/vncviewer/README $(DESTDIR)/$(prefix)/vnc/classes/

xserver-uninstall: uninstall
	rm -f $(DESTDIR)/$(bindir)/Xvnc
	rm -f $(DESTDIR)/$(mandir)/man1/Xvnc.1
	rm -f $(DESTDIR)/$(mandir)/man1/Xserver.1
	rm -f $(DESTDIR)/$(prefix)/vnc/classes/index.vnc
	rm -f $(DESTDIR)/$(prefix)/vnc/classes/VncViewer.jar
	rm -f $(DESTDIR)/$(prefix)/vnc/classes/README
	rmdir $(DESTDIR)/$(prefix)/vnc/classes/
	rmdir $(DESTDIR)/$(prefix)/vnc/
