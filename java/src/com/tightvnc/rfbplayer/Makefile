#
# Making the VNC applet.
#

CP = cp
JC = javac
JAR = jar
ARCHIVE = VncViewer.jar
PAGES = index.vnc shared.vnc noshared.vnc hextile.vnc zlib.vnc tight.vnc
INSTALL_DIR = /usr/local/vnc/classes

CLASSES = VncViewer.class RfbProto.class AuthPanel.class VncCanvas.class \
	  OptionsFrame.class ClipboardFrame.class ButtonPanel.class \
	  DesCipher.class

SOURCES = VncViewer.java RfbProto.java AuthPanel.java VncCanvas.java \
	  OptionsFrame.java ClipboardFrame.java ButtonPanel.java \
	  DesCipher.java

all: $(CLASSES) $(ARCHIVE)

$(CLASSES): $(SOURCES)
	$(JC) -O $(SOURCES)

$(ARCHIVE): $(CLASSES)
	$(JAR) cf $(ARCHIVE) $(CLASSES)

install: $(CLASSES) $(ARCHIVE)
	$(CP) $(CLASSES) $(ARCHIVE) $(PAGES) $(INSTALL_DIR)

export:: $(CLASSES) $(ARCHIVE) $(PAGES)
	@$(ExportJavaClasses)

clean::
	$(RM) *.class *.jar
