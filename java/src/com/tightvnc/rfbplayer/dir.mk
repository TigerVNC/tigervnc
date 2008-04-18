#
# Making the VNC applet.
#

CLASSES = VncViewer.class RfbProto.class AuthPanel.class VncCanvas.class \
	  OptionsFrame.class ClipboardFrame.class ButtonPanel.class \
	  DesCipher.class

PAGES = index.vnc shared.vnc noshared.vnc hextile.vnc zlib.vnc tight.vnc

all: $(CLASSES) VncViewer.jar

VncViewer.jar: $(CLASSES)
	@$(JavaArchive)

export:: $(CLASSES) VncViewer.jar $(PAGES)
	@$(ExportJavaClasses)

clean::
	$(RM) *.class *.jar
