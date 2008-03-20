bin_PROGRAMS = x0vncserver

x0vncserver_SOURCES = buildtime.c Geometry.cxx Geometry.h Image.cxx Image.h \
	PollingManager.cxx PollingManager.h PollingScheduler.cxx \
	PollingScheduler.h TimeMillis.cxx TimeMillis.h XPixelBuffer.cxx \
	XPixelBuffer.h x0vncserver.cxx \
	$(UNIX_DIR)/vncconfig/QueryConnectDialog.cxx

x0vncserver_CPPFLAGS = -I$(UNIX_DIR) -I$(COMMON_DIR) -I$(TX_DIR) \
	-I$(VNCCONFIG_DIR)

x0vncserver_LDADD = $(RFB_LIBS) $(NETWORK_LIBS) $(RDR_LIBS) $(TX_LIBS) \
	$(ZLIB_LIBS) $(JPEG_LIBS) -lXext -lX11
