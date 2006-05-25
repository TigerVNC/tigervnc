# Microsoft Developer Studio Project File - Name="rdr" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=rdr - Win32 Debug Unicode
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "rdr.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "rdr.mak" CFG="rdr - Win32 Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "rdr - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "rdr - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "rdr - Win32 Debug Unicode" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "rdr - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\Release"
# PROP Intermediate_Dir "..\Release\rdr"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O1 /I ".." /FI"rdr/msvcwarning.h" /D "NDEBUG" /D "_LIB" /D "WIN32" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "rdr - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\Debug"
# PROP Intermediate_Dir "..\Debug\rdr"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /FI"rdr/msvcwarning.h" /D "_DEBUG" /D "_LIB" /D "WIN32" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "rdr - Win32 Debug Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "rdr___Win32_Debug_Unicode"
# PROP BASE Intermediate_Dir "rdr___Win32_Debug_Unicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\Debug_Unicode"
# PROP Intermediate_Dir "..\Debug_Unicode\rdr"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /FI"msvcwarning.h" /D "_DEBUG" /D "_LIB" /D "WIN32" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /FI"rdr/msvcwarning.h" /D "_LIB" /D "_DEBUG" /D "WIN32" /D "_UNICODE" /D "UNICODE" /YX /FD /GZ /c
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

# Name "rdr - Win32 Release"
# Name "rdr - Win32 Debug"
# Name "rdr - Win32 Debug Unicode"
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Exception.h
# End Source File
# Begin Source File

SOURCE=.\FdInStream.h
# End Source File
# Begin Source File

SOURCE=.\FdOutStream.h
# End Source File
# Begin Source File

SOURCE=.\FixedMemOutStream.h
# End Source File
# Begin Source File

SOURCE=.\HexInStream.h
# End Source File
# Begin Source File

SOURCE=.\HexOutStream.h
# End Source File
# Begin Source File

SOURCE=.\InStream.h
# End Source File
# Begin Source File

SOURCE=.\MemInStream.h
# End Source File
# Begin Source File

SOURCE=.\MemOutStream.h
# End Source File
# Begin Source File

SOURCE=.\msvcwarning.h
# End Source File
# Begin Source File

SOURCE=.\OutStream.h
# End Source File
# Begin Source File

SOURCE=.\RandomStream.h
# End Source File
# Begin Source File

SOURCE=.\SubstitutingInStream.h
# End Source File
# Begin Source File

SOURCE=.\types.h
# End Source File
# Begin Source File

SOURCE=.\ZlibInStream.h
# End Source File
# Begin Source File

SOURCE=.\ZlibOutStream.h
# End Source File
# End Group
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Exception.cxx
# End Source File
# Begin Source File

SOURCE=.\FdInStream.cxx
# ADD CPP /I "../zlib"
# End Source File
# Begin Source File

SOURCE=.\FdOutStream.cxx
# ADD CPP /I "../zlib"
# End Source File
# Begin Source File

SOURCE=.\HexInStream.cxx
# End Source File
# Begin Source File

SOURCE=.\HexOutStream.cxx
# End Source File
# Begin Source File

SOURCE=.\InStream.cxx
# ADD CPP /I "../zlib"
# End Source File
# Begin Source File

SOURCE=.\RandomStream.cxx
# End Source File
# Begin Source File

SOURCE=.\ZlibInStream.cxx
# ADD CPP /I "../zlib"
# End Source File
# Begin Source File

SOURCE=.\ZlibOutStream.cxx
# ADD CPP /I "../zlib"
# End Source File
# End Group
# End Target
# End Project
