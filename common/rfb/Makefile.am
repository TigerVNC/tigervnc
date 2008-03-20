noinst_LTLIBRARIES = librfb.la

librfb_la_SOURCES = Blacklist.cxx Blacklist.h CapsContainer.cxx \
	CapsContainer.h CapsList.cxx CapsList.h CConnection.cxx CConnection.h \
	CFTMsgReader.cxx CFTMsgReader.h CFTMsgWriter.cxx CFTMsgWriter.h \
	CMsgHandler.cxx CMsgHandler.h CMsgReader.cxx CMsgReader.h \
	CMsgReaderV3.cxx CMsgReaderV3.h CMsgWriter.cxx CMsgWriter.h \
	CMsgWriterV3.cxx CMsgWriterV3.h ColourCube.h ColourMap.h \
	ComparingUpdateTracker.cxx ComparingUpdateTracker.h Configuration.cxx \
	Configuration.h ConnParams.cxx ConnParams.h CSecurity.h \
	CSecurityNone.h CSecurityVncAuth.cxx CSecurityVncAuth.h Cursor.cxx \
	Cursor.h Decoder.cxx Decoder.h DirManager.h d3des.c d3des.h \
	Encoder.cxx Encoder.h encodings.cxx encodings.h Exception.h \
	FileInfo.cxx FileInfo.h FileManager.cxx FileManager.h FileReader.cxx \
	FileReader.h FileWriter.cxx FileWriter.h fttypes.h hextileConstants.h \
	hextileDecode.h HextileDecoder.cxx HextileDecoder.h \
	hextileEncodeBetter.h hextileEncode.h HextileEncoder.cxx \
	HextileEncoder.h Hostname.h HTTPServer.cxx HTTPServer.h ImageGetter.h \
	InputHandler.h JpegCompressor.cxx JpegCompressor.h JpegEncoder.cxx \
	JpegEncoder.h KeyRemapper.cxx KeyRemapper.h keysymdef.h \
	ListConnInfo.h Logger.cxx Logger_file.cxx Logger_file.h Logger.h \
	Logger_stdio.cxx Logger_stdio.h LogWriter.cxx LogWriter.h msgTypes.h \
	Password.cxx Password.h PixelBuffer.cxx PixelBuffer.h PixelFormat.cxx \
	PixelFormat.h Pixel.h RawDecoder.cxx RawDecoder.h RawEncoder.cxx \
	RawEncoder.h Rect.h Region.cxx Region.h rreDecode.h RREDecoder.cxx \
	RREDecoder.h rreEncode.h RREEncoder.cxx RREEncoder.h \
	ScaledPixelBuffer.cxx ScaledPixelBuffer.h ScaleFilters.cxx \
	ScaleFilters.h SConnection.cxx SConnection.h SDesktop.h secTypes.cxx \
	secTypes.h ServerCore.cxx ServerCore.h SFileTransfer.cxx \
	SFileTransfer.h SFileTransferManager.cxx SFileTransferManager.h \
	SFTMsgReader.cxx SFTMsgReader.h SFTMsgWriter.cxx SFTMsgWriter.h \
	SMsgHandler.cxx SMsgHandler.h SMsgReader.cxx SMsgReader.h \
	SMsgReaderV3.cxx SMsgReaderV3.h SMsgWriter.cxx SMsgWriter.h \
	SMsgWriterV3.cxx SMsgWriterV3.h SSecurityFactoryStandard.cxx \
	SSecurityFactoryStandard.h SSecurity.h SSecurityNone.h \
	SSecurityVncAuth.cxx SSecurityVncAuth.h Threading.h tightDecode.h \
	TightDecoder.cxx TightDecoder.h tightEncode.h TightEncoder.cxx \
	TightEncoder.h TightPalette.cxx TightPalette.h Timer.cxx Timer.h \
	TransferQueue.cxx TransferQueue.h TransImageGetter.cxx \
	TransImageGetter.h transInitTempl.h transTempl.h TrueColourMap.h \
	UpdateTracker.cxx UpdateTracker.h UserPasswdGetter.h util.cxx util.h \
	VNCSConnectionST.cxx VNCSConnectionST.h VNCServer.h VNCServerST.cxx \
	VNCServerST.h zrleDecode.h ZRLEDecoder.cxx ZRLEDecoder.h zrleEncode.h \
	ZRLEEncoder.cxx ZRLEEncoder.h

if CL
librfb_la_SOURCES += IrixCLJpegCompressor.cxx IrixCLJpegCompressor.h
endif

if DMEDIA
librfb_la_SOURCES += IrixDMIC_RawToJpeg.cxx IrixDMIC_RawToJpeg.h \
	IrixDMJpegCompressor.cxx IrixDMJpegCompressor.h
endif

librfb_la_CPPFLAGS = -I$(COMMON_DIR)
