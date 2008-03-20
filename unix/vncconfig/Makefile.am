bin_PROGRAMS = vncconfig

vncconfig_SOURCES = buildtime.c QueryConnectDialog.cxx QueryConnectDialog.h \
	vncconfig.cxx vncExt.c vncExt.h

vncconfig_CPPFLAGS = -I$(COMMON_DIR) -I$(TX_DIR)

vncconfig_LDADD = $(TX_LIBS) $(RFB_LIBS) $(NETWORK_LIBS) $(RDR_LIBS) -lX11 \
	-lXext
