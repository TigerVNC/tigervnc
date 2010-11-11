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

CLASSES = VncViewer.class RfbProto.class AuthPanel.class VncCanvas.class \
	  VncCanvas2.class \
	  OptionsFrame.class ClipboardFrame.class ButtonPanel.class \
	  DesCipher.class CapabilityInfo.class CapsContainer.class \
	  RecordingFrame.class SessionRecorder.class \
	  SocketFactory.class HTTPConnectSocketFactory.class \
	  HTTPConnectSocket.class ReloginPanel.class \
	  InStream.class MemInStream.class ZlibInStream.class \
	  Dialog.class MessageBox.class

SOURCES = VncViewer.java RfbProto.java AuthPanel.java VncCanvas.java \
	  VncCanvas2.java \
	  OptionsFrame.java ClipboardFrame.java ButtonPanel.java \
	  DesCipher.java CapabilityInfo.java CapsContainer.java \
	  RecordingFrame.java SessionRecorder.java \
	  SocketFactory.java HTTPConnectSocketFactory.java \
	  HTTPConnectSocket.java ReloginPanel.java \
	  InStream.java MemInStream.java ZlibInStream.java \
	  Dialog.java MessageBox.java

all: $(CLASSES) $(ARCHIVE)

$(CLASSES): $(SOURCES)
	$(JC) $(JCFLAGS) -O $(SOURCES)

$(ARCHIVE): $(CLASSES) $(MANIFEST)
	cd ../../..; \
	$(JAR) cfm com/tigervnc/vncviewer/$(ARCHIVE) \
		com/tigervnc/vncviewer/$(MANIFEST) \
		com/tigervnc/vncviewer/*.class \
		com/tigervnc/decoder/*.class \
		com/tigervnc/decoder/common/*.class

install: $(CLASSES) $(ARCHIVE)
	$(CP) $(CLASSES) $(ARCHIVE) $(PAGES) $(INSTALL_DIR)

export:: $(CLASSES) $(ARCHIVE) $(PAGES)
	@$(ExportJavaClasses)

clean::
	$(RM) *.class *.jar ../decoder/*.class ../decoder/common/*.class
