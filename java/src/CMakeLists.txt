find_package(Java)

set(DEFAULT_JAVACFLAGS "-source 1.5 -target 1.5")
set(JAVACFLAGS ${DEFAULT_JAVACFLAGS} CACHE STRING
  "Java compiler flags (Default: ${DEFAULT_JAVACFLAGS})")
message(STATUS "Java compiler flags = ${JAVACFLAGS}")

set(CLASSPATH com/tigervnc/vncviewer)
set(JAVA_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/${CLASSPATH})
set(JAVA_BINDIR ${CMAKE_CURRENT_BINARY_DIR}/${CLASSPATH})

if(MSVC)
  execute_process(COMMAND "${CMAKE_SOURCE_DIR}/cmake/getdate.bat" "javadate"
    OUTPUT_VARIABLE JAVA_DATE)
  execute_process(COMMAND "${CMAKE_SOURCE_DIR}/cmake/getdate.bat" "javatime"
    OUTPUT_VARIABLE JAVA_TIME)
else()
  execute_process(COMMAND "date" "+%b %d %Y" OUTPUT_VARIABLE JAVA_DATE)
  execute_process(COMMAND "date" "+%H:%M:%S" OUTPUT_VARIABLE JAVA_TIME)
endif()
string(REGEX REPLACE "\n" "" JAVA_DATE ${JAVA_DATE})
string(REGEX REPLACE "\n" "" JAVA_TIME ${JAVA_TIME})

set(JAVA_CLASSNAMES
  CConn
  ClipboardDialog
  DesktopWindow
  Dialog
  F8Menu
  OptionsDialogCallback
  PasswdDialog
  PixelBufferImage
  OptionsDialog
  ServerDialog
  UserPrefs
  VncViewer)

if(MSVC_IDE)
  set(OBJDIR "${CMAKE_CURRENT_BINARY_DIR}/$(OutDir)")
else()
  set(OBJDIR ${CMAKE_CURRENT_BINARY_DIR})
endif()

set(JAVA_SOURCES "")
set(JAVA_CLASSES_FULL "")
foreach(class ${JAVA_CLASSNAMES})
  set(JAVA_SOURCES ${JAVA_SOURCES} ${JAVA_SRCDIR}/${class}.java)
  set(JAVA_CLASSES_FULL ${JAVA_CLASSES_FULL}
    ${OBJDIR}/${CLASSPATH}/${class}.class)
endforeach()

file(GLOB DEPEND_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/com/tigervnc/rfb/*.java
  ${CMAKE_CURRENT_SOURCE_DIR}/com/tigervnc/rdr/*.java
  ${CMAKE_CURRENT_SOURCE_DIR}/com/jcraft/jzlib/*.java)

string(REGEX REPLACE " " ";" JAVACFLAGS "${JAVACFLAGS}")
add_custom_command(OUTPUT ${JAVA_CLASSES_FULL}
  DEPENDS ${JAVA_SOURCES} ${DEPEND_SOURCES}
  COMMAND ${JAVA_COMPILE}
  ARGS ${JAVACFLAGS} -cp ${CMAKE_CURRENT_SOURCE_DIR} -d ${OBJDIR} ${JAVA_SOURCES})

configure_file(${CLASSPATH}/timestamp.in ${CLASSPATH}/timestamp)

add_custom_command(OUTPUT ${JAVA_BINDIR}/tigervnc.png
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${JAVA_SRCDIR}/tigervnc.png ${JAVA_BINDIR}/tigervnc.png
  DEPENDS ${JAVA_SRCDIR}/tigervnc.png)

add_custom_command(OUTPUT ${JAVA_BINDIR}/tigervnc.ico
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${JAVA_SRCDIR}/tigervnc.ico ${JAVA_BINDIR}/tigervnc.ico
  DEPENDS ${JAVA_SRCDIR}/tigervnc.ico)

add_custom_command(OUTPUT VncViewer.jar
  DEPENDS ${JAVA_CLASSES_FULL}
    ${JAVA_SRCDIR}/MANIFEST.MF
    ${JAVA_BINDIR}/timestamp
    ${JAVA_BINDIR}/tigervnc.png
    ${JAVA_BINDIR}/tigervnc.ico
  COMMAND ${JAVA_ARCHIVE}
  ARGS cfm VncViewer.jar
    ${JAVA_SRCDIR}/MANIFEST.MF
    com/tigervnc/vncviewer/timestamp
    com/tigervnc/vncviewer/*.class
    com/tigervnc/rfb/*.class
    com/tigervnc/rdr/*.class
    com/jcraft/jzlib/*.class
    com/tigervnc/vncviewer/tigervnc.png
    com/tigervnc/vncviewer/tigervnc.ico
  WORKING_DIRECTORY ${OBJDIR})

add_custom_target(java ALL DEPENDS VncViewer.jar)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/VncViewer.jar
  ${JAVA_SRCDIR}/README
  ${JAVA_SRCDIR}/index.vnc
  DESTINATION vnc/classes)
