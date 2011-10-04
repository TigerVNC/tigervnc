#
# Making the VNC applet.
#

CP = cp
JC = javac
JCFLAGS = -target 1.5 -Xlint:all,-serial,-cast -classpath ../../../
JAR = jar
ARCHIVE = VncViewer.jar
MANIFEST = MANIFEST.MF
TIMESTAMP = timestamp
PAGES = index.vnc
INSTALL_DIR = /usr/local/vnc/classes
DATE = $(shell date -u "+%b %d %Y")
TIME = $(shell date -u "+%H:%M:%S")

CLASSES = CConn.class OptionsDialogCallback.class ClipboardDialog.class \
    PasswdDialog.class DesktopWindow.class PixelBufferImage.class \
    Dialog.class ServerDialog.class F8Menu.class	UserPrefs.class \
    OptionsDialog.class VncViewer.class

SOURCES = CConn.java OptionsDialogCallback.java ClipboardDialog.java \
    PasswdDialog.java DesktopWindow.java PixelBufferImage.java \
    Dialog.java ServerDialog.java F8Menu.java	UserPrefs.java \
    OptionsDialog.java VncViewer.java

all: $(CLASSES) $(ARCHIVE)

$(CLASSES): $(SOURCES) ../rfb/*.java ../rdr/*.java ../../jcraft/jzlib/*.java
	$(JC) $(JCFLAGS) -O $(SOURCES)

$(ARCHIVE): $(CLASSES) $(MANIFEST) $(TIMESTAMP)
	cd ../../..; \
	$(JAR) cfm com/tigervnc/vncviewer/$(ARCHIVE) \
		com/tigervnc/vncviewer/$(MANIFEST) \
		com/tigervnc/vncviewer/$(TIMESTAMP) \
		com/tigervnc/vncviewer/*.class \
		com/tigervnc/rfb/*.class \
		com/tigervnc/rdr/*.class \
		com/jcraft/jzlib/*.class \
		com/tigervnc/vncviewer/tigervnc.png \
		com/tigervnc/vncviewer/tigervnc.ico

$(TIMESTAMP):
	$(shell cat $(MANIFEST) > $(TIMESTAMP))
	$(shell echo "Package-Date: $(DATE)" >> $(TIMESTAMP))
	$(shell echo "Package-Time: $(TIME)" >> $(TIMESTAMP))

install: $(PAGES) README $(ARCHIVE)
	mkdir -p $(INSTALL_DIR)
	$(CP) $(ARCHIVE) $(PAGES) README $(INSTALL_DIR)

export:: $(CLASSES) $(ARCHIVE) $(PAGES)
	@$(ExportJavaClasses)

clean::
	$(RM) *.class *.jar timestamp ../rfb/*.class ../rdr/*.class ../../jcraft/jzlib/*.class
