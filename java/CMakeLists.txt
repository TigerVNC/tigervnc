cmake_minimum_required(VERSION 3.10.0)

project(tigervnc-java Java)
if(NOT VERSION)
  set(VERSION 1.13.1)
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
set(JAVA_TSA_URL NOTFOUND CACHE STRING "URL of Time Stamping Authority (TSA)")

if(NOT BUILD)
	STRING(TIMESTAMP BUILD "%Y%m%d" UTC)
endif()
STRING(TIMESTAMP JAVA_DATE "%Y-%m-%d" UTC)
STRING(TIMESTAMP JAVA_TIME "%H:%M:%S" UTC)

set(JAVA_SOURCES "")
set(JAVA_CLASSES "")

set(JSCH_CLASSNAMES
  DH
  DHEC256
  DHEC384
  DHEC521
  DHECN
  DHGEX256
  DHG1
  DHG14
  DHGEX
  ECDH
  JSch
  Session
  UserAuth
  UserAuthKeyboardInteractive
  UserAuthPassword
  UserAuthPublicKey
  UserAuthNone
  jce/AES128CBC
  jce/AES128CTR
  jce/AES192CBC
  jce/AES192CTR
  jce/AES256CBC
  jce/AES256CTR
  jce/ARCFOUR
  jce/ARCFOUR128
  jce/ARCFOUR256
  jce/BlowfishCBC
  jce/DH
	jce/ECDH256
	jce/ECDH384
	jce/ECDH521
	jce/ECDHN
  jce/HMAC
  jce/HMACMD5
  jce/HMACMD596
  jce/HMACSHA1
  jce/HMACSHA196
  jce/HMACSHA256
  jce/HMACSHA512
  jce/KeyPairGenDSA
  jce/KeyPairGenECDSA
  jce/KeyPairGenRSA
  jce/MD5
  jce/PBKDF
  jce/Random
  jce/SHA1
  jce/SHA256
  jce/SHA384
  jce/SHA512
  jce/SignatureDSA
  jce/SignatureECDSA
  jce/SignatureRSA
  jce/TripleDESCBC
  jce/TripleDESCTR
  jcraft/Compression
  jcraft/HMAC
  jcraft/HMACMD596
  jcraft/HMACMD5
  jcraft/HMACSHA196
  jcraft/HMACSHA1)

foreach(class ${JSCH_CLASSNAMES})
  set(JAVA_SOURCES ${JAVA_SOURCES} com/jcraft/jsch/${class}.java)
  set(JAVA_CLASSES ${JAVA_CLASSES}
    ${BINDIR}/com/jcraft/jsch/${class}.class)
endforeach()

file(GLOB DEPEND_SOURCES
  ${SRCDIR}/${CLASSPATH}/*.java
  ${SRCDIR}/com/tigervnc/rfb/*.java
  ${SRCDIR}/com/tigervnc/rdr/*.java
  ${SRCDIR}/com/tigervnc/network/*.java
  ${SRCDIR}/com/jcraft/jsch/*.java
  ${SRCDIR}/com/jcraft/jsch/jcraft/*.java
  ${SRCDIR}/com/jcraft/jsch/jgss/*.java
  ${SRCDIR}/com/jcraft/jzlib/*.java)

string(REGEX REPLACE " " ";" JAVACFLAGS "${JAVACFLAGS}")
add_custom_command(OUTPUT ${JAVA_CLASSES}
  DEPENDS ${JAVA_SOURCES} ${DEPEND_SOURCES}
  COMMAND ${JAVA_COMPILE}
  ARGS ${JAVACFLAGS} -sourcepath ${SRCDIR} -d ${BINDIR} 
    ${CLASSPATH}/VncViewer.java ${JAVA_SOURCES}
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
    com/jcraft/jzlib/*.class
    com/jcraft/jsch/jcraft/*.class
    com/jcraft/jsch/jce/*.class
    com/jcraft/jsch/*.class
    com/tigervnc/vncviewer/*.png
    com/tigervnc/vncviewer/tigervnc.ico
  COMMAND ${CMAKE_COMMAND}
  ARGS -DJava_PATH=${Java_PATH} -DJAR_FILE=${BINDIR}/VncViewer.jar
    -DJAVA_KEYSTORE=${JAVA_KEYSTORE}
    -DJAVA_KEYSTORE_TYPE=${JAVA_KEYSTORE_TYPE}
    -DJAVA_STOREPASS=${JAVA_STOREPASS}
    -DJAVA_KEYPASS=${JAVA_KEYPASS}
    -DJAVA_KEY_ALIAS=${JAVA_KEY_ALIAS}
    -DJAVA_TSA_URL=${JAVA_TSA_URL}
    -P ${SRCDIR}/cmake/SignJar.cmake)

add_custom_target(java ALL DEPENDS VncViewer.jar)
