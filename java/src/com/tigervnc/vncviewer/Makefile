#
# Making the VNC applet.
#

CP = cp
JC = javac
JCFLAGS = -target 1.5 -classpath ../../../
JAR = jar
ARCHIVE = VncViewer.jar
MANIFEST = MANIFEST.MF
PAGES = index.vnc
INSTALL_DIR = /usr/local/vnc/classes

CLASSES = CConn.class OptionsDialogCallback.class ClipboardDialog.class \
    PasswdDialog.class DesktopWindow.class PixelBufferImage.class \
    Dialog.class ServerDialog.class F8Menu.class	UserPrefs.class \
    OptionsDialog.class VncViewer.class

SOURCES = CConn.java OptionsDialogCallback.java ClipboardDialog.java \
    PasswdDialog.java DesktopWindow.java PixelBufferImage.java \
    Dialog.java ServerDialog.java F8Menu.java	UserPrefs.java \
    OptionsDialog.java VncViewer.java

all: $(CLASSES) $(ARCHIVE)

$(CLASSES): $(SOURCES)
	$(JC) $(JCFLAGS) -O $(SOURCES)

$(ARCHIVE): $(CLASSES) $(MANIFEST)
	cd ../../..; \
	$(JAR) cfm com/tigervnc/vncviewer/$(ARCHIVE) \
		com/tigervnc/vncviewer/$(MANIFEST) \
		com/tigervnc/vncviewer/*.class \
		com/tigervnc/rfb/*.class \
		com/tigervnc/rdr/*.class \
		com/tigervnc/vncviewer/tigervnc.png \
		com/tigervnc/vncviewer/tigervnc.ico

install: $(CLASSES) $(ARCHIVE)
	$(CP) $(CLASSES) $(ARCHIVE) $(PAGES) $(INSTALL_DIR)

export:: $(CLASSES) $(ARCHIVE) $(PAGES)
	@$(ExportJavaClasses)

clean::
	$(RM) *.class *.jar ../rfb/*.class ../rdr/*.class
