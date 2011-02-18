#
# Setup
#

cmake_minimum_required(VERSION 2.6)

project(libjpeg-turbo)
set(VERSION 1.0.90)

if(NOT DEFINED WITH_SIMD)
  set(WITH_SIMD 1)
endif()

# Detect whether compiler is 64-bit
if(MSVC AND CMAKE_CL_64)
  set(SIMD_X86_64 1)
  set(64BIT 1)
elseif(CMAKE_SIZEOF_VOID_P MATCHES 8)
  set(SIMD_X86_64 1)
  set(64BIT 1)
endif()

if(64BIT)
  message(STATUS "64-bit build")
else()
  message(STATUS "32-bit build")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/win)


#
# Targets
#

set(JPEG_SOURCES jcapimin.c jcapistd.c jccoefct.c jccolor.c jcdctmgr.c jchuff.c
  jcinit.c jcmainct.c jcmarker.c jcmaster.c jcomapi.c jcparam.c jcphuff.c
  jcprepct.c jcsample.c jctrans.c jdapimin.c jdapistd.c jdatadst.c jdatasrc.c
  jdcoefct.c jdcolor.c jddctmgr.c jdhuff.c jdinput.c jdmainct.c jdmarker.c
  jdmaster.c jdmerge.c jdphuff.c jdpostct.c jdsample.c jdtrans.c jerror.c
  jfdctflt.c jfdctfst.c jfdctint.c jidctflt.c jidctfst.c jidctint.c jidctred.c
  jquant1.c jquant2.c jutils.c jmemmgr.c jmemnobs.c)

if(WITH_SIMD)
  add_definitions(-DWITH_SIMD)
  add_subdirectory(simd)
  if(SIMD_X86_64)
    set(JPEG_SOURCES ${JPEG_SOURCES} simd/jsimd_x86_64.c)
  else()
    set(JPEG_SOURCES ${JPEG_SOURCES} simd/jsimd_i386.c)
  endif()
  # This tells CMake that the "source" files haven't been generated yet
  set_source_files_properties(${SIMD_OBJS} PROPERTIES GENERATED 1)
else()
  set(JPEG_SOURCES ${JPEG_SOURCES} jsimd_none.c)
  message(STATUS "Not using SIMD acceleration")
endif()

add_library(jpeg STATIC ${JPEG_SOURCES} ${SIMD_OBJS})
if(WITH_SIMD)
  add_dependencies(jpeg simd)
endif()

add_executable(jpegut jpegut.c turbojpegl.c)
target_link_libraries(jpegut jpeg)

add_executable(jpgtest jpgtest.cxx bmp.c turbojpegl.c)
target_link_libraries(jpgtest jpeg)

add_executable(cjpeg cjpeg.c cdjpeg.c rdbmp.c rdgif.c rdppm.c rdswitch.c
  rdtarga.c)
set_property(TARGET cjpeg PROPERTY COMPILE_FLAGS
  "-DBMP_SUPPORTED -DGIF_SUPPORTED -DPPM_SUPPORTED -DTARGA_SUPPORTED")
target_link_libraries(cjpeg jpeg)

add_executable(djpeg djpeg.c cdjpeg.c rdcolmap.c rdswitch.c wrbmp.c wrgif.c
  wrppm.c wrtarga.c)
set_property(TARGET djpeg PROPERTY COMPILE_FLAGS
  "-DBMP_SUPPORTED -DGIF_SUPPORTED -DPPM_SUPPORTED -DTARGA_SUPPORTED")
target_link_libraries(djpeg jpeg)

add_executable(jpegtran jpegtran.c cdjpeg.c rdswitch.c transupp.c)
target_link_libraries(jpegtran jpeg)


#
# Tests
#

enable_testing()
add_test(jpegut jpegut)
add_test(cjpeg-int cjpeg -dct int -outfile testoutint.jpg ${CMAKE_CURRENT_SOURCE_DIR}/testorig.ppm)
add_test(cjpeg-int-cmp ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/testimgint.jpg testoutint.jpg)
add_test(cjpeg-fast cjpeg -dct fast -opt -outfile testoutfst.jpg ${CMAKE_CURRENT_SOURCE_DIR}/testorig.ppm)
add_test(cjpeg-fast-cmp ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/testimgfst.jpg testoutfst.jpg)
add_test(cjpeg-fast-100 cjpeg -dct fast -quality 100 -opt -outfile testoutfst100.jpg ${CMAKE_CURRENT_SOURCE_DIR}/testorig.ppm)
add_test(cjpeg-fast-100-cmp ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/testimgfst100.jpg testoutfst100.jpg)
add_test(cjpeg-float cjpeg -dct float -outfile testoutflt.jpg ${CMAKE_CURRENT_SOURCE_DIR}/testorig.ppm)
if(WITH_SIMD)
add_test(cjpeg-float-cmp ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/testimgflt.jpg testoutflt.jpg)
else()
add_test(cjpeg-float-cmp ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/testimgflt-nosimd.jpg testoutflt.jpg)
endif()
add_test(djpeg-int djpeg -dct int -fast -ppm -outfile testoutint.ppm ${CMAKE_CURRENT_SOURCE_DIR}/testorig.jpg)
add_test(djpeg-int-cmp ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/testimgint.ppm testoutint.ppm)
add_test(djpeg-fast djpeg -dct fast -ppm -outfile testoutfst.ppm ${CMAKE_CURRENT_SOURCE_DIR}/testorig.jpg)
add_test(djpeg-fast-cmp ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/testimgfst.ppm testoutfst.ppm)
add_test(djpeg-float djpeg -dct float -ppm -outfile testoutflt.ppm ${CMAKE_CURRENT_SOURCE_DIR}/testorig.jpg)
if(WITH_SIMD)
add_test(djpeg-float-cmp ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/testimgflt.ppm testoutflt.ppm)
else()
add_test(djpeg-float-cmp ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/testorig.ppm testoutflt.ppm)
endif()
add_test(djpeg-256 djpeg -dct int -bmp -colors 256 -outfile testout.bmp  ${CMAKE_CURRENT_SOURCE_DIR}/testorig.jpg)
add_test(djpeg-256-cmp ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/testimg.bmp testout.bmp)
add_test(cjpeg-prog cjpeg -dct int -progressive -outfile testoutp.jpg ${CMAKE_CURRENT_SOURCE_DIR}/testorig.ppm)
add_test(cjpeg-prog-cmp ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/testimgp.jpg testoutp.jpg)
add_test(jpegtran-prog jpegtran -outfile testoutt.jpg testoutp.jpg)
add_test(jpegtran-prog-cmp ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/testimgint.jpg testoutt.jpg)
