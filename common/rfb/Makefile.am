noinst_LTLIBRARIES = librfb.la

VENCRYPT_HDRS = CSecurityTLS.h SSecurityTLS.h

VENCRYPT_SRCS = CSecurityTLS.cxx SSecurityTLS.cxx

HDRS = Blacklist.h CapsContainer.h CapsList.h CConnection.h \
	CMsgHandler.h CMsgReader.h CMsgReaderV3.h CMsgWriter.h \
	CMsgWriterV3.h ColourCube.h ColourMap.h ComparingUpdateTracker.h \
	Configuration.h ConnParams.h CSecurity.h CSecurityNone.h \
	CSecurityPlain.h CSecurityStack.h CSecurityVeNCrypt.h \
	CSecurityVncAuth.h Cursor.h Decoder.h d3des.h Encoder.h \
	encodings.h Exception.h hextileConstants.h hextileDecode.h \
	HextileDecoder.h hextileEncodeBetter.h hextileEncode.h \
	HextileEncoder.h Hostname.h HTTPServer.h ImageGetter.h InputHandler.h \
	KeyRemapper.h keysymdef.h ListConnInfo.h Logger.h Logger_file.h \
	Logger_stdio.h LogWriter.h msgTypes.h Password.h PixelBuffer.h \
	PixelFormat.h PixelFormat.inl Pixel.h RawDecoder.h RawEncoder.h \
	Rect.h Region.h rreDecode.h RREDecoder.h rreEncode.h RREEncoder.h \
	ScaledPixelBuffer.h ScaleFilters.h SConnection.h ScreenSet.h \
	screenTypes.h SDesktop.h Security.h SecurityServer.h SecurityClient.h \
	ServerCore.h SMsgHandler.h \
	SMsgReader.h SMsgReaderV3.h SMsgWriter.h SMsgWriterV3.h SSecurity.h \
	SSecurityNone.h SSecurityPlain.h SSecurityStack.h SSecurityVeNCrypt.h \
	SSecurityVncAuth.h Threading.h tightDecode.h \
	TightDecoder.h tightEncode.h TightEncoder.h TightPalette.h Timer.h \
	TransImageGetter.h transInitTempl.h transTempl.h TrueColourMap.h \
	UpdateTracker.h UserMsgBox.h UserPasswdGetter.h util.h \
	VNCSConnectionST.h VNCServer.h VNCServerST.h zrleDecode.h \
	ZRLEDecoder.h zrleEncode.h ZRLEEncoder.h

librfb_la_SOURCES = $(HDRS) Blacklist.cxx CConnection.cxx CMsgHandler.cxx \
	CMsgReader.cxx CMsgReaderV3.cxx CMsgWriter.cxx CMsgWriterV3.cxx \
	CSecurityPlain.cxx CSecurityStack.cxx CSecurityVeNCrypt.cxx \
	CSecurityVncAuth.cxx CapsContainer.cxx CapsList.cxx \
	ComparingUpdateTracker.cxx Configuration.cxx ConnParams.cxx \
	Cursor.cxx Decoder.cxx d3des.c Encoder.cxx \
	HTTPServer.cxx HextileDecoder.cxx HextileEncoder.cxx \
	KeyRemapper.cxx LogWriter.cxx Logger.cxx Logger_file.cxx \
	Logger_stdio.cxx Password.cxx PixelBuffer.cxx PixelFormat.cxx \
	RREEncoder.cxx RREDecoder.cxx RawDecoder.cxx RawEncoder.cxx \
	Region.cxx SConnection.cxx SMsgHandler.cxx \
	SMsgReader.cxx SMsgReaderV3.cxx SMsgWriter.cxx SMsgWriterV3.cxx \
	ServerCore.cxx Security.cxx SecurityServer.cxx SecurityClient.cxx \
	SSecurityPlain.cxx SSecurityStack.cxx \
	SSecurityVncAuth.cxx SSecurityVeNCrypt.cxx \
	ScaledPixelBuffer.cxx ScaleFilters.cxx Timer.cxx TightDecoder.cxx \
	TightEncoder.cxx TightPalette.cxx TransImageGetter.cxx \
	UpdateTracker.cxx VNCSConnectionST.cxx \
	VNCServerST.cxx ZRLEEncoder.cxx ZRLEDecoder.cxx encodings.cxx \
	util.cxx

if HAVE_GNUTLS
librfb_la_SOURCES += $(VENCRYPT_HDRS) $(VENCRYPT_SRCS)
endif

librfb_la_CPPFLAGS = -I$(top_srcdir)/common -I$(top_srcdir)/win
librfb_la_LIBADD = @GNUTLS_LIBS@

if HAVE_PAM
librfb_la_SOURCES += UnixPasswordValidator.cxx UnixPasswordValidator.h pam.c pam.h
librfb_la_LIBADD += $(PAM_LIBS)
endif

if INCLUDED_JPEG
librfb_la_CPPFLAGS += -I$(top_srcdir)/common/jpeg -I$(top_builddir)/common/jpeg
librfb_la_LIBADD += $(top_builddir)/common/jpeg/libjpeg.la
endif

if BUILD_WIN
librfb_la_SOURCES += WinPasswdValidator.h WinPasswdValidator.cxx
endif

EXTRA_DIST = CMakeLists.txt
