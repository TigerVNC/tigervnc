cmake_minimum_required(VERSION 3.10.0)

project(tigervnc-java Java)
if(NOT VERSION)
  set(VERSION 1.16.80)
endif()

find_package(Java)

set(DEFAULT_JAVACFLAGS "-source 8 -target 8 -encoding UTF-8 -Xlint:all,-serial,-cast,-unchecked,-fallthrough,-dep-ann,-deprecation,-rawtypes")
set(JAVACFLAGS ${DEFAULT_JAVACFLAGS} CACHE STRING
  "Java compiler flags (Default: ${DEFAULT_JAVACFLAGS})")
message(STATUS "Java compiler flags = ${JAVACFLAGS}")

set(CLASSPATH com/tigervnc/vncviewer)
set(SRCDIR ${CMAKE_CURRENT_SOURCE_DIR})
set(BINDIR ${CMAKE_CURRENT_BINARY_DIR})
set(JAVA_KEYSTORE NOTFOUND CACHE STRING "URL of keystore location")
set(JAVA_KEYSTORE_TYPE "jks" CACHE STRING "Type of keystore (Default: \"jks\")")
set(JAVA_KEY_ALIAS NOTFOUND CACHE STRING "Alias for the keystore entry used to generate the signature")
set(JAVA_STOREPASS NOTFOUND CACHE STRING "Password required to access the keystore")
set(JAVA_KEYPASS NOTFOUND CACHE STRING "Password used to protect the private key of the specified keystore entry")
set(JAVA_PKCS11_PROVIDER_CLASS "sun.security.pkcs11.SunPKCS11" CACHE STRING "PKCS11 SecurityProvider class name")
set(JAVA_PKCS11_PROVIDER_ARG NOTFOUND CACHE STRING "Path to the PKCS11 security provider class config file")
set(JAVA_TSA_URL NOTFOUND CACHE STRING "URL of Time Stamping Authority (TSA)")
set(JAVA_CERT_CHAIN NOTFOUND CACHE STRING "Path to CA certificate chain file")
option(JAVA_SIGN_JAR "Sign Java viewer jar" ON)

if(NOT BUILD)
	STRING(TIMESTAMP BUILD "%Y%m%d" UTC)
endif()
STRING(TIMESTAMP JAVA_DATE "%Y-%m-%d" UTC)
STRING(TIMESTAMP JAVA_TIME "%H:%M:%S" UTC)

file(GLOB DEPEND_SOURCES
  ${SRCDIR}/${CLASSPATH}/*.java
  ${SRCDIR}/com/tigervnc/rfb/*.java
  ${SRCDIR}/com/tigervnc/rdr/*.java
  ${SRCDIR}/com/tigervnc/network/*.java)

file(GLOB JSCH_SOURCES
  ${SRCDIR}/com/jcraft/jsch/*.java)
list(FILTER JSCH_SOURCES EXCLUDE REGEX "SSHAgentConnector|WindowsSSHAgentConnector|PageantConnector|JUnixSocketFactory")

file(GLOB PROVIDER_SOURCES
  ${SRCDIR}/com/jcraft/jsch/jce/*.java
  ${SRCDIR}/com/jcraft/jsch/bc/*.java
  ${SRCDIR}/com/jcraft/jsch/jgss/*.java
  ${SRCDIR}/com/jcraft/jsch/juz/*.java
  ${SRCDIR}/com/jcraft/jsch/jbcrypt/*.java)

set(JAVA_CLASSES ${BINDIR}/${CLASSPATH}/VncViewer.class)

string(REGEX REPLACE " " ";" JAVACFLAGS "${JAVACFLAGS}")
add_custom_command(OUTPUT ${JAVA_CLASSES}
  DEPENDS ${DEPEND_SOURCES} ${JSCH_SOURCES} ${PROVIDER_SOURCES}
  COMMAND ${JAVA_COMPILE}
  ARGS ${JAVACFLAGS} -sourcepath ${SRCDIR} -d ${BINDIR} 
    ${DEPEND_SOURCES} ${JSCH_SOURCES} ${PROVIDER_SOURCES}
	WORKING_DIRECTORY ${SRCDIR})

configure_file(${CLASSPATH}/timestamp.in ${CLASSPATH}/timestamp)

if(NOT "${SRCDIR}" STREQUAL "${BINDIR}")

set(ICONS
tigervnc.ico
tigervnc.png
insecure.png
secure.png)

foreach(icon ${ICONS})
  add_custom_command(OUTPUT ${BINDIR}/${CLASSPATH}/${icon}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${SRCDIR}/${CLASSPATH}/${icon} ${BINDIR}/${CLASSPATH}/${icon}
    DEPENDS ${SRCDIR}/${CLASSPATH}/${icon})
endforeach()

endif()

string(REGEX REPLACE "jar" "" Java_PATH ${Java_JAR_EXECUTABLE})
string(REGEX REPLACE ".exe" "" Java_PATH ${Java_PATH})

set(SIGN_JAR_COMMAND)
if(JAVA_SIGN_JAR)
  set(SIGN_JAR_COMMAND
    COMMAND ${CMAKE_COMMAND}
    ARGS -DJava_PATH=${Java_PATH} -DJAR_FILE=${BINDIR}/VncViewer.jar
      -DJAVA_KEYSTORE=${JAVA_KEYSTORE}
      -DJAVA_KEYSTORE_TYPE=${JAVA_KEYSTORE_TYPE}
      -DJAVA_STOREPASS=${JAVA_STOREPASS}
      -DJAVA_PKCS11_PROVIDER_CLASS=${JAVA_PKCS11_PROVIDER_CLASS}
      -DJAVA_PKCS11_PROVIDER_ARG=${JAVA_PKCS11_PROVIDER_ARG}
      -DJAVA_KEYPASS=${JAVA_KEYPASS}
      -DJAVA_KEY_ALIAS=${JAVA_KEY_ALIAS}
      -DJAVA_TSA_URL=${JAVA_TSA_URL}
      -DJAVA_CERT_CHAIN=${JAVA_CERT_CHAIN}
      -P ${SRCDIR}/cmake/SignJar.cmake)
else()
  message(STATUS "Java viewer jar signing disabled")
endif()

add_custom_command(OUTPUT VncViewer.jar
  DEPENDS ${JAVA_CLASSES}
    ${SRCDIR}/${CLASSPATH}/MANIFEST.MF
    ${BINDIR}/${CLASSPATH}/timestamp
    ${BINDIR}/${CLASSPATH}/tigervnc.ico
    ${BINDIR}/${CLASSPATH}/tigervnc.png
    ${BINDIR}/${CLASSPATH}/insecure.png
    ${BINDIR}/${CLASSPATH}/secure.png
  COMMAND ${JAVA_ARCHIVE}
  ARGS cfm VncViewer.jar
    ${SRCDIR}/${CLASSPATH}/MANIFEST.MF
    ${CLASSPATH}/timestamp
    ${CLASSPATH}/*.class
    com/tigervnc/rfb/*.class
    com/tigervnc/rdr/*.class
    com/tigervnc/network/*.class
    com/jcraft/jsch/*.class
    com/jcraft/jsch/*/*.class
    com/tigervnc/vncviewer/*.png
    com/tigervnc/vncviewer/tigervnc.ico
  ${SIGN_JAR_COMMAND})

add_custom_target(java ALL DEPENDS VncViewer.jar)
