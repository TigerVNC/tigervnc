#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/CConn.o \
	${OBJECTDIR}/DesktopWindow.o \
	${OBJECTDIR}/OptionsDialog.o \
	${OBJECTDIR}/PlatformPixelBuffer.o \
	${OBJECTDIR}/ServerDialog.o \
	${OBJECTDIR}/Surface.o \
	${OBJECTDIR}/Surface_OSX.o \
	${OBJECTDIR}/Surface_Win32.o \
	${OBJECTDIR}/Surface_X11.o \
	${OBJECTDIR}/UserDialog.o \
	${OBJECTDIR}/Viewport.o \
	${OBJECTDIR}/cocoa.o \
	${OBJECTDIR}/menukey.o \
	${OBJECTDIR}/parameters.o \
	${OBJECTDIR}/vncviewer.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/vncviewer

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/vncviewer: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/vncviewer ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/CConn.o: CConn.cxx
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../common -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CConn.o CConn.cxx

${OBJECTDIR}/DesktopWindow.o: DesktopWindow.cxx
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../common -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/DesktopWindow.o DesktopWindow.cxx

${OBJECTDIR}/OptionsDialog.o: OptionsDialog.cxx
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../common -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/OptionsDialog.o OptionsDialog.cxx

${OBJECTDIR}/PlatformPixelBuffer.o: PlatformPixelBuffer.cxx
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../common -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/PlatformPixelBuffer.o PlatformPixelBuffer.cxx

${OBJECTDIR}/ServerDialog.o: ServerDialog.cxx
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../common -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/ServerDialog.o ServerDialog.cxx

${OBJECTDIR}/Surface.o: Surface.cxx
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../common -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Surface.o Surface.cxx

${OBJECTDIR}/Surface_OSX.o: Surface_OSX.cxx
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../common -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Surface_OSX.o Surface_OSX.cxx

${OBJECTDIR}/Surface_Win32.o: Surface_Win32.cxx
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../common -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Surface_Win32.o Surface_Win32.cxx

${OBJECTDIR}/Surface_X11.o: Surface_X11.cxx
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../common -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Surface_X11.o Surface_X11.cxx

${OBJECTDIR}/UserDialog.o: UserDialog.cxx
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../common -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/UserDialog.o UserDialog.cxx

${OBJECTDIR}/Viewport.o: Viewport.cxx
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../common -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Viewport.o Viewport.cxx

${OBJECTDIR}/cocoa.o: cocoa.mm
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../common -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/cocoa.o cocoa.mm

${OBJECTDIR}/menukey.o: menukey.cxx
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../common -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/menukey.o menukey.cxx

${OBJECTDIR}/parameters.o: parameters.cxx
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../common -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/parameters.o parameters.cxx

${OBJECTDIR}/vncviewer.o: vncviewer.cxx
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../common -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/vncviewer.o vncviewer.cxx

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
