cmake_minimum_required(VERSION 2.8)

project(tigervnc-java Java)
if(NOT VERSION)
  set(VERSION 1.2.80)
endif()

find_package(Java)

set(DEFAULT_JAVACFLAGS "-source 1.5 -target 1.5 -Xlint:all,-serial,-cast,-unchecked,-fallthrough,-dep-ann,-deprecation")
set(JAVACFLAGS ${DEFAULT_JAVACFLAGS} CACHE STRING
  "Java compiler flags (Default: ${DEFAULT_JAVACFLAGS})")
message(STATUS "Java compiler flags = ${JAVACFLAGS}")

set(CLASSPATH com/tigervnc/vncviewer)
set(JAVA_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/${CLASSPATH})
set(JAVA_BINDIR ${CMAKE_CURRENT_BINARY_DIR}/${CLASSPATH})

if(NOT BUILD)
	execute_process(COMMAND "date" "+%Y%m%d" OUTPUT_VARIABLE BUILD)
endif()
execute_process(COMMAND "date" "+%b %d %Y" OUTPUT_VARIABLE JAVA_DATE)
execute_process(COMMAND "date" "+%H:%M:%S" OUTPUT_VARIABLE JAVA_TIME)
string(REGEX REPLACE "\n" "" JAVA_DATE ${JAVA_DATE})
string(REGEX REPLACE "\n" "" JAVA_TIME ${JAVA_TIME})
string(REGEX REPLACE "\n" "" BUILD ${BUILD})

set(JAVA_CLASSNAMES
  CConn
  ClipboardDialog
  DesktopWindow
  Dialog
  F8Menu
  OptionsDialogCallback
  PasswdDialog
  PlatformPixelBuffer
  OptionsDialog
  ServerDialog
  VncViewer)

set(JAVA_SOURCES "")
set(JAVA_CLASSES_FULL "")
foreach(class ${JAVA_CLASSNAMES})
  set(JAVA_SOURCES ${JAVA_SOURCES} ${JAVA_SRCDIR}/${class}.java)
  set(JAVA_CLASSES_FULL ${JAVA_CLASSES_FULL}
    ${CMAKE_CURRENT_BINARY_DIR}/${CLASSPATH}/${class}.class)
endforeach()

set(JSCH_CLASSNAMES
  DH
  DHG1
  DHG14
  DHGEX
  JSch
  Session
  UserAuth
  UserAuthKeyboardInteractive
  UserAuthPassword
  UserAuthPublicKey
  UserAuthNone
  jce/AES128CBC
  jce/AES192CTR
  jce/ARCFOUR128
  jce/BlowfishCBC
  jce/HMACMD5
  jce/KeyPairGenDSA
  jce/Random
  jce/SignatureRSA
  jce/AES128CTR
  jce/AES256CBC
  jce/ARCFOUR256
  jce/DH
  jce/HMACSHA196
  jce/KeyPairGenRSA
  jce/SHA1
  jce/TripleDESCBC
  jce/AES192CBC
  jce/AES256CTR
  jce/ARCFOUR
  jce/HMACMD596
  jce/HMACSHA1
  jce/MD5
  jce/SignatureDSA
  jce/TripleDESCTR
  jcraft/Compression
  jcraft/HMAC
  jcraft/HMACMD596
  jcraft/HMACMD5
  jcraft/HMACSHA196
  jcraft/HMACSHA1)
set(JSCH_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/com/jcraft/jsch)
foreach(class ${JSCH_CLASSNAMES})
  set(JAVA_SOURCES ${JAVA_SOURCES} ${JSCH_SRCDIR}/${class}.java)
  set(JAVA_CLASSES_FULL ${JAVA_CLASSES_FULL}
    ${CMAKE_CURRENT_BINARY_DIR}/com/jcraft/jsch/${class}.class)
endforeach()

file(GLOB DEPEND_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/com/tigervnc/rfb/*.java
  ${CMAKE_CURRENT_SOURCE_DIR}/com/tigervnc/rdr/*.java
  ${CMAKE_CURRENT_SOURCE_DIR}/com/tigervnc/network/*.java
  ${CMAKE_CURRENT_SOURCE_DIR}/com/jcraft/jsch/*.java
  ${CMAKE_CURRENT_SOURCE_DIR}/com/jcraft/jsch/jcraft/*.java
  ${CMAKE_CURRENT_SOURCE_DIR}/com/jcraft/jsch/jgss/*.java
  ${CMAKE_CURRENT_SOURCE_DIR}/com/jcraft/jzlib/*.java)

string(REGEX REPLACE " " ";" JAVACFLAGS "${JAVACFLAGS}")
add_custom_command(OUTPUT ${JAVA_CLASSES_FULL}
  DEPENDS ${JAVA_SOURCES} ${DEPEND_SOURCES}
  COMMAND ${JAVA_COMPILE}
  ARGS ${JAVACFLAGS} -sourcepath ${CMAKE_CURRENT_SOURCE_DIR}
    -d ${CMAKE_CURRENT_BINARY_DIR} ${JAVA_SOURCES})

configure_file(${CLASSPATH}/timestamp.in ${CLASSPATH}/timestamp)

add_custom_command(OUTPUT ${JAVA_BINDIR}/tigervnc.png
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${JAVA_SRCDIR}/tigervnc.png ${JAVA_BINDIR}/tigervnc.png
  DEPENDS ${JAVA_SRCDIR}/tigervnc.png)

add_custom_command(OUTPUT ${JAVA_BINDIR}/tigervnc.ico
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${JAVA_SRCDIR}/tigervnc.ico ${JAVA_BINDIR}/tigervnc.ico
  DEPENDS ${JAVA_SRCDIR}/tigervnc.ico)

string(REGEX REPLACE "jar" "" Java_PATH ${Java_JAR_EXECUTABLE})
string(REGEX REPLACE ".exe" "" Java_PATH ${Java_PATH})

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
    com/tigervnc/network/*.class
    com/jcraft/jzlib/*.class
    com/jcraft/jsch/jcraft/*.class
    com/jcraft/jsch/jce/*.class
    com/jcraft/jsch/*.class
    com/tigervnc/vncviewer/tigervnc.png
    com/tigervnc/vncviewer/tigervnc.ico
  COMMAND ${CMAKE_COMMAND}
  ARGS -DJava_PATH=${Java_PATH}
    -DJAR_FILE=${CMAKE_CURRENT_BINARY_DIR}/VncViewer.jar
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/SignJar.cmake)

add_custom_target(java ALL DEPENDS VncViewer.jar)

if(NOT WIN32)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/VncViewer.jar
    ${JAVA_SRCDIR}/README
    ${JAVA_SRCDIR}/index.vnc
    ${JAVA_SRCDIR}/favicon.ico
    DESTINATION ${DATA_DIR}/vnc/classes)
endif()
