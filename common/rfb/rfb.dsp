# Microsoft Developer Studio Project File - Name="rfb" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=rfb - Win32 Debug Unicode
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "rfb.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "rfb.mak" CFG="rfb - Win32 Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "rfb - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "rfb - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "rfb - Win32 Debug Unicode" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "rfb - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\Release"
# PROP Intermediate_Dir "..\Release\rfb"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O1 /I ".." /I "../../win" /I "../jpeg" /I "../jpeg/win" /FI"rdr/msvcwarning.h" /D "NDEBUG" /D "_LIB" /D "WIN32" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "rfb - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\Debug"
# PROP Intermediate_Dir "..\Debug\rfb"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /I "../../win" /I "../jpeg" /I "../jpeg/win" /FI"rdr/msvcwarning.h" /D "_DEBUG" /D "_LIB" /D "WIN32" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "rfb - Win32 Debug Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "rfb___Win32_Debug_Unicode"
# PROP BASE Intermediate_Dir "rfb___Win32_Debug_Unicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\Debug_Unicode"
# PROP Intermediate_Dir "..\Debug_Unicode\rfb"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /FI"msvcwarning.h" /D "_DEBUG" /D "_LIB" /D "WIN32" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /I "../../win" /I "../jpeg" /I "../jpeg/win" /FI"rdr/msvcwarning.h" /D "_LIB" /D "_DEBUG" /D "WIN32" /D "_UNICODE" /D "UNICODE" /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "rfb - Win32 Release"
# Name "rfb - Win32 Debug"
# Name "rfb - Win32 Debug Unicode"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Blacklist.cxx
# End Source File
# Begin Source File

SOURCE=.\CapsContainer.cxx
# End Source File
# Begin Source File

SOURCE=.\CapsList.cxx
# End Source File
# Begin Source File

SOURCE=.\CConnection.cxx
# End Source File
# Begin Source File

SOURCE=.\CFTMsgReader.cxx
# End Source File
# Begin Source File

SOURCE=.\CFTMsgWriter.cxx
# End Source File
# Begin Source File

SOURCE=.\CMsgHandler.cxx
# End Source File
# Begin Source File

SOURCE=.\CMsgReader.cxx
# End Source File
# Begin Source File

SOURCE=.\CMsgReaderV3.cxx
# End Source File
# Begin Source File

SOURCE=.\CMsgWriter.cxx
# End Source File
# Begin Source File

SOURCE=.\CMsgWriterV3.cxx
# End Source File
# Begin Source File

SOURCE=.\ComparingUpdateTracker.cxx
# End Source File
# Begin Source File

SOURCE=.\Configuration.cxx
# End Source File
# Begin Source File

SOURCE=.\ConnParams.cxx
# End Source File
# Begin Source File

SOURCE=.\CSecurityVncAuth.cxx
# End Source File
# Begin Source File

SOURCE=.\Cursor.cxx
# End Source File
# Begin Source File

SOURCE=.\d3des.c
# End Source File
# Begin Source File

SOURCE=.\Decoder.cxx
# End Source File
# Begin Source File

SOURCE=.\Encoder.cxx
# ADD CPP /I "../jpeg"
# End Source File
# Begin Source File

SOURCE=.\encodings.cxx
# End Source File
# Begin Source File

SOURCE=.\FileInfo.cxx
# End Source File
# Begin Source File

SOURCE=.\FileManager.cxx
# End Source File
# Begin Source File

SOURCE=.\FileReader.cxx
# End Source File
# Begin Source File

SOURCE=.\FileWriter.cxx
# End Source File
# Begin Source File

SOURCE=.\HextileDecoder.cxx
# End Source File
# Begin Source File

SOURCE=.\HextileEncoder.cxx
# End Source File
# Begin Source File

SOURCE=.\HTTPServer.cxx
# End Source File
# Begin Source File

SOURCE=.\JpegCompressor.cxx
# End Source File
# Begin Source File

SOURCE=.\JpegEncoder.cxx
# End Source File
# Begin Source File

SOURCE=.\KeyRemapper.cxx
# End Source File
# Begin Source File

SOURCE=.\Logger.cxx
# End Source File
# Begin Source File

SOURCE=.\Logger_file.cxx
# End Source File
# Begin Source File

SOURCE=.\Logger_stdio.cxx
# End Source File
# Begin Source File

SOURCE=.\LogWriter.cxx
# End Source File
# Begin Source File

SOURCE=.\Password.cxx
# End Source File
# Begin Source File

SOURCE=.\PixelBuffer.cxx
# End Source File
# Begin Source File

SOURCE=.\PixelFormat.cxx
# End Source File
# Begin Source File

SOURCE=.\RawDecoder.cxx
# End Source File
# Begin Source File

SOURCE=.\RawEncoder.cxx
# End Source File
# Begin Source File

SOURCE=.\Region.cxx
# End Source File
# Begin Source File

SOURCE=.\RREDecoder.cxx
# End Source File
# Begin Source File

SOURCE=.\RREEncoder.cxx
# End Source File
# Begin Source File

SOURCE=.\ScaledPixelBuffer.cxx
# End Source File
# Begin Source File

SOURCE=.\ScaleFilters.cxx
# End Source File
# Begin Source File

SOURCE=.\SConnection.cxx
# End Source File
# Begin Source File

SOURCE=.\secTypes.cxx
# End Source File
# Begin Source File

SOURCE=.\ServerCore.cxx
# End Source File
# Begin Source File

SOURCE=.\SFileTransfer.cxx
# End Source File
# Begin Source File

SOURCE=.\SFileTransferManager.cxx
# End Source File
# Begin Source File

SOURCE=.\SFTMsgReader.cxx
# End Source File
# Begin Source File

SOURCE=.\SFTMsgWriter.cxx
# End Source File
# Begin Source File

SOURCE=.\SMsgHandler.cxx
# End Source File
# Begin Source File

SOURCE=.\SMsgReader.cxx
# End Source File
# Begin Source File

SOURCE=.\SMsgReaderV3.cxx
# End Source File
# Begin Source File

SOURCE=.\SMsgWriter.cxx
# End Source File
# Begin Source File

SOURCE=.\SMsgWriterV3.cxx
# End Source File
# Begin Source File

SOURCE=.\SSecurityFactoryStandard.cxx
# End Source File
# Begin Source File

SOURCE=.\SSecurityVncAuth.cxx
# End Source File
# Begin Source File

SOURCE=.\TightDecoder.cxx
# ADD CPP /I "../jpeg"
# End Source File
# Begin Source File

SOURCE=.\TightEncoder.cxx
# ADD CPP /I "../jpeg"
# End Source File
# Begin Source File

SOURCE=.\TightPalette.cxx
# End Source File
# Begin Source File

SOURCE=.\TransferQueue.cxx
# End Source File
# Begin Source File

SOURCE=.\TransImageGetter.cxx
# End Source File
# Begin Source File

SOURCE=.\UpdateTracker.cxx
# End Source File
# Begin Source File

SOURCE=.\util.cxx
# End Source File
# Begin Source File

SOURCE=.\VNCSConnectionST.cxx
# End Source File
# Begin Source File

SOURCE=.\VNCServerST.cxx
# End Source File
# Begin Source File

SOURCE=.\ZRLEDecoder.cxx
# End Source File
# Begin Source File

SOURCE=.\ZRLEEncoder.cxx
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Blacklist.h
# End Source File
# Begin Source File

SOURCE=.\CapsContainer.h
# End Source File
# Begin Source File

SOURCE=.\CapsList.h
# End Source File
# Begin Source File

SOURCE=.\CConnection.h
# End Source File
# Begin Source File

SOURCE=.\CFTMsgReader.h
# End Source File
# Begin Source File

SOURCE=.\CFTMsgWriter.h
# End Source File
# Begin Source File

SOURCE=.\CMsgHandler.h
# End Source File
# Begin Source File

SOURCE=.\CMsgReader.h
# End Source File
# Begin Source File

SOURCE=.\CMsgReaderV3.h
# End Source File
# Begin Source File

SOURCE=.\CMsgWriter.h
# End Source File
# Begin Source File

SOURCE=.\CMsgWriterV3.h
# End Source File
# Begin Source File

SOURCE=.\ColourCube.h
# End Source File
# Begin Source File

SOURCE=.\ColourMap.h
# End Source File
# Begin Source File

SOURCE=.\ComparingUpdateTracker.h
# End Source File
# Begin Source File

SOURCE=.\Configuration.h
# End Source File
# Begin Source File

SOURCE=.\ConnParams.h
# End Source File
# Begin Source File

SOURCE=.\CSecurity.h
# End Source File
# Begin Source File

SOURCE=.\CSecurityNone.h
# End Source File
# Begin Source File

SOURCE=.\CSecurityVncAuth.h
# End Source File
# Begin Source File

SOURCE=.\Cursor.h
# End Source File
# Begin Source File

SOURCE=.\d3des.h
# End Source File
# Begin Source File

SOURCE=.\Decoder.h
# End Source File
# Begin Source File

SOURCE=.\DirManager.h
# End Source File
# Begin Source File

SOURCE=.\Encoder.h
# End Source File
# Begin Source File

SOURCE=.\encodings.h
# End Source File
# Begin Source File

SOURCE=.\Exception.h
# End Source File
# Begin Source File

SOURCE=.\FileInfo.h
# End Source File
# Begin Source File

SOURCE=.\FileManager.h
# End Source File
# Begin Source File

SOURCE=.\FileReader.h
# End Source File
# Begin Source File

SOURCE=.\FileWriter.h
# End Source File
# Begin Source File

SOURCE=.\hextileConstants.h
# End Source File
# Begin Source File

SOURCE=.\hextileDecode.h
# End Source File
# Begin Source File

SOURCE=.\HextileDecoder.h
# End Source File
# Begin Source File

SOURCE=.\hextileEncode.h
# End Source File
# Begin Source File

SOURCE=.\hextileEncodeBetter.h
# End Source File
# Begin Source File

SOURCE=.\HextileEncoder.h
# End Source File
# Begin Source File

SOURCE=.\Hostname.h
# End Source File
# Begin Source File

SOURCE=.\HTTPServer.h
# End Source File
# Begin Source File

SOURCE=.\ImageGetter.h
# End Source File
# Begin Source File

SOURCE=.\InputHandler.h
# End Source File
# Begin Source File

SOURCE=.\JpegCompressor.h
# End Source File
# Begin Source File

SOURCE=.\JpegEncoder.h
# End Source File
# Begin Source File

SOURCE=.\KeyRemapper.h
# End Source File
# Begin Source File

SOURCE=.\keysymdef.h
# End Source File
# Begin Source File

SOURCE=.\ListConnInfo.h
# End Source File
# Begin Source File

SOURCE=.\Logger.h
# End Source File
# Begin Source File

SOURCE=.\Logger_file.h
# End Source File
# Begin Source File

SOURCE=.\Logger_stdio.h
# End Source File
# Begin Source File

SOURCE=.\LogWriter.h
# End Source File
# Begin Source File

SOURCE=.\msgTypes.h
# End Source File
# Begin Source File

SOURCE=.\msvcwarning.h
# End Source File
# Begin Source File

SOURCE=.\Password.h
# End Source File
# Begin Source File

SOURCE=.\Pixel.h
# End Source File
# Begin Source File

SOURCE=.\PixelBuffer.h
# End Source File
# Begin Source File

SOURCE=.\PixelFormat.h
# End Source File
# Begin Source File

SOURCE=.\RawDecoder.h
# End Source File
# Begin Source File

SOURCE=.\RawEncoder.h
# End Source File
# Begin Source File

SOURCE=.\Rect.h
# End Source File
# Begin Source File

SOURCE=.\Region.h
# End Source File
# Begin Source File

SOURCE=.\rreDecode.h
# End Source File
# Begin Source File

SOURCE=.\RREDecoder.h
# End Source File
# Begin Source File

SOURCE=.\rreEncode.h
# End Source File
# Begin Source File

SOURCE=.\RREEncoder.h
# End Source File
# Begin Source File

SOURCE=.\ScaledPixelBuffer.h
# End Source File
# Begin Source File

SOURCE=.\ScaleFilters.h
# End Source File
# Begin Source File

SOURCE=.\SConnection.h
# End Source File
# Begin Source File

SOURCE=.\SDesktop.h
# End Source File
# Begin Source File

SOURCE=.\secTypes.h
# End Source File
# Begin Source File

SOURCE=.\ServerCore.h
# End Source File
# Begin Source File

SOURCE=.\SFileTransfer.h
# End Source File
# Begin Source File

SOURCE=.\SFileTransferManager.h
# End Source File
# Begin Source File

SOURCE=.\SFTMsgReader.h
# End Source File
# Begin Source File

SOURCE=.\SFTMsgWriter.h
# End Source File
# Begin Source File

SOURCE=.\SMsgHandler.h
# End Source File
# Begin Source File

SOURCE=.\SMsgReader.h
# End Source File
# Begin Source File

SOURCE=.\SMsgReaderV3.h
# End Source File
# Begin Source File

SOURCE=.\SMsgWriter.h
# End Source File
# Begin Source File

SOURCE=.\SMsgWriterV3.h
# End Source File
# Begin Source File

SOURCE=.\SSecurity.h
# End Source File
# Begin Source File

SOURCE=.\SSecurityFactoryStandard.h
# End Source File
# Begin Source File

SOURCE=.\SSecurityNone.h
# End Source File
# Begin Source File

SOURCE=.\SSecurityVncAuth.h
# End Source File
# Begin Source File

SOURCE=.\Threading.h
# End Source File
# Begin Source File

SOURCE=.\tightDecode.h
# End Source File
# Begin Source File

SOURCE=.\TightDecoder.h
# End Source File
# Begin Source File

SOURCE=.\tightEncode.h
# End Source File
# Begin Source File

SOURCE=.\TightEncoder.h
# End Source File
# Begin Source File

SOURCE=.\TightPalette.h
# End Source File
# Begin Source File

SOURCE=.\TransferQueue.h
# End Source File
# Begin Source File

SOURCE=.\TransImageGetter.h
# End Source File
# Begin Source File

SOURCE=.\transInitTempl.h
# End Source File
# Begin Source File

SOURCE=.\transTempl.h
# End Source File
# Begin Source File

SOURCE=.\TrueColourMap.h
# End Source File
# Begin Source File

SOURCE=.\UpdateTracker.h
# End Source File
# Begin Source File

SOURCE=.\UserPasswdGetter.h
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# Begin Source File

SOURCE=.\VNCSConnectionST.h
# End Source File
# Begin Source File

SOURCE=.\VNCServer.h
# End Source File
# Begin Source File

SOURCE=.\VNCServerST.h
# End Source File
# Begin Source File

SOURCE=.\zrleDecode.h
# End Source File
# Begin Source File

SOURCE=.\ZRLEDecoder.h
# End Source File
# Begin Source File

SOURCE=.\zrleEncode.h
# End Source File
# Begin Source File

SOURCE=.\ZRLEEncoder.h
# End Source File
# End Group
# End Target
# End Project
