# Microsoft Developer Studio Project File - Name="vncviewer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=vncviewer - Win32 Debug Unicode
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vncviewer.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vncviewer.mak" CFG="vncviewer - Win32 Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vncviewer - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "vncviewer - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "vncviewer - Win32 Debug Unicode" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vncviewer - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\Release"
# PROP Intermediate_Dir "..\Release\vncviewer"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I ".." /I "../../common" /FI"rdr/msvcwarning.h" /D "NDEBUG" /D "_WINDOWS" /D "WIN32" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 user32.lib gdi32.lib advapi32.lib ws2_32.lib version.lib comctl32.lib shell32.lib comdlg32.lib /nologo /subsystem:windows /machine:I386
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Desc=Updating buildTime
PreLink_Cmds=cl /c /nologo /Fo..\Release\ /Fd..\Release\vncviewer /MT buildTime.cxx
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\Debug"
# PROP Intermediate_Dir "..\Debug\vncviewer"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /I "../../common" /FI"rdr/msvcwarning.h" /D "_DEBUG" /D "_WINDOWS" /D "WIN32" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 user32.lib gdi32.lib advapi32.lib ws2_32.lib version.lib comctl32.lib shell32.lib comdlg32.lib /nologo /subsystem:windows /incremental:no /debug /machine:I386 /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Desc=Updating buildTime
PreLink_Cmds=cl /c /nologo /Fo..\Debug\ /Fd..\Debug\vncviewer /MTd buildTime.cxx
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vncviewer - Win32 Debug Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vncviewer___Win32_Debug_Unicode"
# PROP BASE Intermediate_Dir "vncviewer___Win32_Debug_Unicode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\Debug_Unicode"
# PROP Intermediate_Dir "..\Debug_Unicode\vncviewer"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /FI"msvcwarning.h" /D "_DEBUG" /D "_WINDOWS" /D "WIN32" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /I "../../common" /FI"rdr/msvcwarning.h" /D "_WINDOWS" /D "_DEBUG" /D "WIN32" /D "_UNICODE" /D "UNICODE" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 user32.lib gdi32.lib advapi32.lib ws2_32.lib version.lib comctl32.lib shell32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 user32.lib gdi32.lib advapi32.lib ws2_32.lib version.lib comctl32.lib shell32.lib comdlg32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Desc=Updating buildTime
PreLink_Cmds=cl /c /nologo /Fo..\Debug_Unicode\ /Fd..\Debug_Unicode\vncviewer /MTd buildTime.cxx
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "vncviewer - Win32 Release"
# Name "vncviewer - Win32 Debug"
# Name "vncviewer - Win32 Debug Unicode"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\buildTime.cxx
# End Source File
# Begin Source File

SOURCE=.\CConn.cxx
# End Source File
# Begin Source File

SOURCE=.\CConnOptions.cxx
# End Source File
# Begin Source File

SOURCE=.\CConnThread.cxx
# End Source File
# Begin Source File

SOURCE=.\ConnectingDialog.cxx
# End Source File
# Begin Source File

SOURCE=.\ConnectionDialog.cxx
# End Source File
# Begin Source File

SOURCE=.\DesktopWindow.cxx
# End Source File
# Begin Source File

SOURCE=.\FileTransfer.cxx
# End Source File
# Begin Source File

SOURCE=.\FTBrowseDlg.cxx
# End Source File
# Begin Source File

SOURCE=.\FTDialog.cxx
# End Source File
# Begin Source File

SOURCE=.\FTListView.cxx
# End Source File
# Begin Source File

SOURCE=.\FTProgress.cxx
# End Source File
# Begin Source File

SOURCE=.\InfoDialog.cxx
# End Source File
# Begin Source File

SOURCE=.\OptionsDialog.cxx
# End Source File
# Begin Source File

SOURCE=.\UserPasswdDialog.cxx
# End Source File
# Begin Source File

SOURCE=.\ViewerToolBar.cxx
# End Source File
# Begin Source File

SOURCE=.\vncviewer.cxx
# End Source File
# Begin Source File

SOURCE=.\vncviewer.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CConn.h
# End Source File
# Begin Source File

SOURCE=.\CConnOptions.h
# End Source File
# Begin Source File

SOURCE=.\CConnThread.h
# End Source File
# Begin Source File

SOURCE=.\ConnectingDialog.h
# End Source File
# Begin Source File

SOURCE=.\ConnectionDialog.h
# End Source File
# Begin Source File

SOURCE=.\DesktopWindow.h
# End Source File
# Begin Source File

SOURCE=.\FileTransfer.h
# End Source File
# Begin Source File

SOURCE=.\FTBrowseDlg.h
# End Source File
# Begin Source File

SOURCE=.\FTDialog.h
# End Source File
# Begin Source File

SOURCE=.\FTListView.h
# End Source File
# Begin Source File

SOURCE=.\FTProgress.h
# End Source File
# Begin Source File

SOURCE=.\InfoDialog.h
# End Source File
# Begin Source File

SOURCE=.\ListenServer.h
# End Source File
# Begin Source File

SOURCE=.\ListenTrayIcon.h
# End Source File
# Begin Source File

SOURCE=.\MRU.h
# End Source File
# Begin Source File

SOURCE=.\OptionsDialog.h
# End Source File
# Begin Source File

SOURCE=.\UserPasswdDialog.h
# End Source File
# Begin Source File

SOURCE=.\ViewerToolBar.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\cursor1.cur
# End Source File
# Begin Source File

SOURCE=.\ftdir.ico
# End Source File
# Begin Source File

SOURCE=.\ftfile.ico
# End Source File
# Begin Source File

SOURCE=.\ftreload.ico
# End Source File
# Begin Source File

SOURCE=.\ftup.ico
# End Source File
# Begin Source File

SOURCE=.\toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\vncviewer.bmp
# End Source File
# Begin Source File

SOURCE=.\vncviewer.exe.manifest
# End Source File
# Begin Source File

SOURCE=.\vncviewer.ico
# End Source File
# End Group
# End Target
# End Project
