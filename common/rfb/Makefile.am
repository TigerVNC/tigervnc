noinst_LTLIBRARIES = librfb.la

HDRS = Blacklist.h CapsContainer.h CapsList.h CConnection.h CFTMsgReader.h \
	CFTMsgWriter.h CMsgHandler.h CMsgReader.h CMsgReaderV3.h CMsgWriter.h \
	CMsgWriterV3.h ColourCube.h ColourMap.h ComparingUpdateTracker.h \
	Configuration.h ConnParams.h CSecurity.h CSecurityNone.h \
	CSecurityVncAuth.h Cursor.h Decoder.h DirManager.h d3des.h Encoder.h \
	encodings.h Exception.h FileInfo.h FileManager.h FileReader.h \
	FileWriter.h fttypes.h hextileConstants.h hextileDecode.h \
	HextileDecoder.h hextileEncodeBetter.h hextileEncode.h \
	HextileEncoder.h Hostname.h HTTPServer.h ImageGetter.h InputHandler.h \
	IrixCLJpegCompressor.h IrixDMIC_RawToJpeg.h IrixDMJpegCompressor.h \
	JpegCompressor.h JpegEncoder.h KeyRemapper.h keysymdef.h \
	ListConnInfo.h Logger_file.h Logger.h Logger_stdio.h LogWriter.h \
	msgTypes.h Password.h PixelBuffer.h PixelFormat.h Pixel.h \
	RawDecoder.h RawEncoder.h Rect.h Region.h rreDecode.h RREDecoder.h \
	rreEncode.h RREEncoder.h ScaledPixelBuffer.h ScaleFilters.h \
	SConnection.h SDesktop.h secTypes.h ServerCore.h SFileTransfer.h \
	SFileTransferManager.h SFTMsgReader.h SFTMsgWriter.h SMsgHandler.h \
	SMsgReader.h SMsgReaderV3.h SMsgWriter.h SMsgWriterV3.h \
	SSecurityFactoryStandard.h SSecurity.h SSecurityNone.h \
	SSecurityVncAuth.h Threading.h tightDecode.h TightDecoder.h \
	tightEncode.h TightEncoder.h TightPalette.h Timer.h TransferQueue.h \
	TransImageGetter.h transInitTempl.h transTempl.h TrueColourMap.h \
	UpdateTracker.h UserPasswdGetter.h util.h VNCSConnectionST.h \
	VNCServer.h VNCServerST.h zrleDecode.h ZRLEDecoder.h zrleEncode.h \
	ZRLEEncoder.h

librfb_la_SOURCES = $(HDRS) Blacklist.cxx CConnection.cxx CFTMsgReader.cxx CFTMsgWriter.cxx CMsgHandler.cxx \
	CMsgReader.cxx CMsgReaderV3.cxx CMsgWriter.cxx CMsgWriterV3.cxx \
	CSecurityVncAuth.cxx CapsContainer.cxx CapsList.cxx \
	ComparingUpdateTracker.cxx Configuration.cxx ConnParams.cxx \
	Cursor.cxx Decoder.cxx d3des.c Encoder.cxx FileInfo.cxx \
	FileManager.cxx FileReader.cxx FileWriter.cxx JpegCompressor.cxx \
	JpegEncoder.cxx HTTPServer.cxx HextileDecoder.cxx HextileEncoder.cxx \
	KeyRemapper.cxx LogWriter.cxx Logger.cxx Logger_file.cxx \
	Logger_stdio.cxx Password.cxx PixelBuffer.cxx PixelFormat.cxx \
	RREEncoder.cxx RREDecoder.cxx RawDecoder.cxx RawEncoder.cxx \
	Region.cxx SConnection.cxx SFTMsgReader.cxx SFTMsgWriter.cxx \
	SFileTransfer.cxx SFileTransferManager.cxx SMsgHandler.cxx \
	SMsgReader.cxx SMsgReaderV3.cxx SMsgWriter.cxx SMsgWriterV3.cxx \
	ServerCore.cxx SSecurityFactoryStandard.cxx SSecurityVncAuth.cxx \
	ScaledPixelBuffer.cxx ScaleFilters.cxx Timer.cxx TightDecoder.cxx \
	TightEncoder.cxx TightPalette.cxx TransImageGetter.cxx \
	TransferQueue.cxx UpdateTracker.cxx VNCSConnectionST.cxx \
	VNCServerST.cxx ZRLEEncoder.cxx ZRLEDecoder.cxx encodings.cxx \
	secTypes.cxx util.cxx

librfb_la_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/../win
librfb_la_LIBADD =

if IRIX_COMPRESS
librfb_la_SOURCES += IrixCLJpegCompressor.cxx
librfb_la_LIBADD += -lcl
endif
if IRIX_MEDIA
librfb_la_SOURCES += IrixDMJpegCompressor.cxx IrixDMIC_RawToJpeg.cxx
librfb_la_LIBADD += -ldmedia
endif

if INCLUDED_JPEG
librfb_la_CPPFLAGS += -I$(top_srcdir)/jpeg
librfb_la_LIBADD += $(top_srcdir)/jpeg/libjpeg.la
endif
