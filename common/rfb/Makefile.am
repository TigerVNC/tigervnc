noinst_LTLIBRARIES = librfb.la

librfb_la_SOURCES = Blacklist.cxx CConnection.cxx CMsgHandler.cxx \
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

if IRIX_COMPRESS
librfb_la_SOURCES += IrixCLJpegCompressor.cxx
endif
if IRIX_MEDIA
librfb_la_SOURCES += IrixDMJpegCompressor.cxx IrixDMIC_RawToJpeg.cxx
endif

librfb_la_CPPFLAGS = -I$(top_srcdir) @JPEG_INCLUDE@ @VSNPRINTF_DEFINE@ \
	@STRCASECMP_DEFINE@ @STRNCASECMP_DEFINE@

librfb_la_LIBADD = @JPEG_LIB@
