# Microsoft Developer Studio Project File - Name="rfbplayer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=rfbplayer - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "rfbplayer.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "rfbplayer.mak" CFG="rfbplayer - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "rfbplayer - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "rfbplayer - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "rfbplayer - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\Release"
# PROP Intermediate_Dir "..\Release\rfbplayer"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I ".." /I "../../common" /FI"rdr/msvcwarning.h" /D "NDEBUG" /D "_WINDOWS" /D "WIN32" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 user32.lib gdi32.lib Advapi32.lib comctl32.lib shell32.lib comdlg32.lib version.lib /nologo /subsystem:windows /machine:I386
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Cmds=cl /c /nologo /Fo..\Release\ /Fd..\Release\rfbplayer /MTd buildTime.cxx
# End Special Build Tool

!ELSEIF  "$(CFG)" == "rfbplayer - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\Debug"
# PROP Intermediate_Dir "..\Debug\rfbplayer"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /I "../../common" /FI"rdr/msvcwarning.h" /D "_DEBUG" /D "_WINDOWS" /D "WIN32" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 user32.lib gdi32.lib advapi32.lib ws2_32.lib version.lib comctl32.lib shell32.lib comdlg32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Cmds=cl /c /nologo /Fo..\Debug\ /Fd..\Debug\rfbplayer /MTd buildTime.cxx
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "rfbplayer - Win32 Release"
# Name "rfbplayer - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\buildTime.cxx
# End Source File
# Begin Source File

SOURCE=.\FbsInputStream.cxx
# End Source File
# Begin Source File

SOURCE=.\PixelFormatList.cxx
# End Source File
# Begin Source File

SOURCE=.\PlayerOptions.cxx
# End Source File
# Begin Source File

SOURCE=.\PlayerToolBar.cxx
# End Source File
# Begin Source File

SOURCE=.\rfbplayer.cxx
# End Source File
# Begin Source File

SOURCE=.\rfbplayer.rc
# End Source File
# Begin Source File

SOURCE=.\RfbProto.cxx
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ChoosePixelFormatDialog.h
# End Source File
# Begin Source File

SOURCE=.\EditPixelFormatDialog.h
# End Source File
# Begin Source File

SOURCE=.\FbsInputStream.h
# End Source File
# Begin Source File

SOURCE=.\GotoPosDialog.h
# End Source File
# Begin Source File

SOURCE=.\InfoDialog.h
# End Source File
# Begin Source File

SOURCE=.\OptionsDialog.h
# End Source File
# Begin Source File

SOURCE=.\PixelFormatList.h
# End Source File
# Begin Source File

SOURCE=.\PlayerOptions.h
# End Source File
# Begin Source File

SOURCE=.\PlayerToolBar.h
# End Source File
# Begin Source File

SOURCE=.\rfbplayer.h
# End Source File
# Begin Source File

SOURCE=.\RfbProto.h
# End Source File
# Begin Source File

SOURCE=.\rfbSessionReader.h
# End Source File
# Begin Source File

SOURCE=.\SessionInfoDialog.h
# End Source File
# Begin Source File

SOURCE=.\UserPixelFormatsDialog.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\rfbplayer.ico
# End Source File
# Begin Source File

SOURCE=.\toolbar.bmp
# End Source File
# End Group
# End Target
# End Project
