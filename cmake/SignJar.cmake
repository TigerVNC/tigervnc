if(NOT Java_PATH)
  message(FATAL_ERROR "Java_PATH must be defined")
endif()

if(NOT JAR_FILE)
  message(FATAL_ERROR "JAR_FILE must be defined")
endif()

set(KEYTOOL "${Java_PATH}/keytool")
set(JARSIGNER "${Java_PATH}/jarsigner")

file(REMOVE tigervnc.keystore)
execute_process(COMMAND
  ${KEYTOOL} -genkey -alias TigerVNC -keystore tigervnc.keystore -keyalg RSA
    -storepass tigervnc -keypass tigervnc -validity 7300
    -dname "CN=TigerVNC, OU=Software Development, O=The TigerVNC Project, L=Austin, S=Texas, C=US"
  RESULT_VARIABLE RESULT OUTPUT_VARIABLE OUTPUT ERROR_VARIABLE ERROR)
if(NOT RESULT EQUAL 0)
  message(FATAL_ERROR "${KEYTOOL} failed:\n${ERROR}")
endif()
execute_process(COMMAND
  ${JARSIGNER} -keystore tigervnc.keystore
    -storepass tigervnc -keypass tigervnc ${JAR_FILE} TigerVNC
  RESULT_VARIABLE RESULT OUTPUT_VARIABLE OUTPUT ERROR_VARIABLE ERROR)
if(NOT RESULT EQUAL 0)
  message(FATAL_ERROR "${JARSIGNER} failed:\n${ERROR}")
endif()
file(REMOVE tigervnc.keystore)
