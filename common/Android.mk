LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := cxx

LOCAL_SRC_FILES := \
    network/TcpSocket.cxx \
    os/Mutex.cxx \
    os/os.cxx \
    os/Thread.cxx \
    rdr/Exception.cxx \
    rdr/FdInStream.cxx \
    rdr/FdOutStream.cxx \
    rdr/FileInStream.cxx \
    rdr/HexInStream.cxx \
    rdr/HexOutStream.cxx \
    rdr/InStream.cxx \
    rdr/RandomStream.cxx \
    rdr/TLSException.cxx \
    rdr/TLSInStream.cxx \
    rdr/TLSOutStream.cxx \
    rdr/ZlibInStream.cxx \
    rdr/ZlibOutStream.cxx \
    rfb/Blacklist.cxx \
    rfb/CConnection.cxx \
    rfb/CMsgHandler.cxx \
    rfb/CMsgReader.cxx \
    rfb/CMsgWriter.cxx \
    rfb/ComparingUpdateTracker.cxx \
    rfb/Configuration.cxx \
    rfb/ConnParams.cxx \
    rfb/CopyRectDecoder.cxx \
    rfb/CSecurityPlain.cxx \
    rfb/CSecurityStack.cxx \
    rfb/CSecurityVeNCrypt.cxx \
    rfb/CSecurityVncAuth.cxx \
    rfb/Cursor.cxx \
    rfb/DecodeManager.cxx \
    rfb/Decoder.cxx \
    rfb/EncodeManager.cxx \
    rfb/Encoder.cxx \
    rfb/encodings.cxx \
    rfb/HextileDecoder.cxx \
    rfb/HextileEncoder.cxx \
    rfb/HTTPServer.cxx \
    rfb/JpegCompressor.cxx \
    rfb/JpegDecompressor.cxx \
    rfb/KeyRemapper.cxx \
    rfb/Logger.cxx \
    rfb/Logger_android.cxx \
    rfb/Logger_stdio.cxx \
    rfb/LogWriter.cxx \
    rfb/Password.cxx \
    rfb/PixelBuffer.cxx \
    rfb/PixelFormat.cxx \
    rfb/RawDecoder.cxx \
    rfb/RawEncoder.cxx \
    rfb/Region.cxx \
    rfb/RREDecoder.cxx \
    rfb/RREEncoder.cxx \
    rfb/ScaleFilters.cxx \
    rfb/SConnection.cxx \
    rfb/SecurityClient.cxx \
    rfb/Security.cxx \
    rfb/SecurityServer.cxx \
    rfb/ServerCore.cxx \
    rfb/SMsgHandler.cxx \
    rfb/SMsgReader.cxx \
    rfb/SMsgWriter.cxx \
    rfb/SSecurityPlain.cxx \
    rfb/SSecurityStack.cxx \
    rfb/SSecurityVeNCrypt.cxx \
    rfb/SSecurityVncAuth.cxx \
    rfb/TightDecoder.cxx \
    rfb/TightEncoder.cxx \
    rfb/TightJPEGEncoder.cxx \
    rfb/Timer.cxx \
    rfb/UpdateTracker.cxx \
    rfb/util.cxx \
    rfb/VNCSConnectionST.cxx \
    rfb/VNCServerST.cxx \
    rfb/ZRLEDecoder.cxx \
    rfb/ZRLEEncoder.cxx \
    rfb/d3des.c \
    Xregion/Region.c

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    external/libjpeg-turbo \
	external/zlib

LOCAL_CFLAGS := -Ofast -Wall -Wformat=2 -DNDEBUG -UNDEBUG -Werror
LOCAL_CFLAGS += -Wno-unused-parameter

LOCAL_CPPFLAGS := -std=c++11 -fexceptions -frtti

LOCAL_SHARED_LIBRARIES := \
    libjpeg \
    libz

LOCAL_MODULE := libtigervnc
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)
