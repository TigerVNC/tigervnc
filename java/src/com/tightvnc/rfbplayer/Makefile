#
# Making the VNC applet.
#

CP = cp
RM = rm
JC = javac
JAR = jar
ARCHIVE = RfbPlayer.jar
PAGES = index.html
INSTALL_DIR = ./classes

CLASSES = RfbPlayer.class RfbProto.class ButtonPanel.class VncCanvas.class \
	  FbsInputStream.class

SOURCES = RfbPlayer.java RfbProto.java ButtonPanel.java VncCanvas.java \
	  FbsInputStream.java

all: $(CLASSES) $(ARCHIVE)

$(CLASSES): $(SOURCES)
	$(JC) -g $(SOURCES)

$(ARCHIVE): $(CLASSES)
	$(JAR) cf $(ARCHIVE) $(CLASSES)

install: $(CLASSES) $(ARCHIVE)
	$(CP) $(CLASSES) $(ARCHIVE) $(PAGES) $(INSTALL_DIR)

clean::
	$(RM) *.class *.jar
