# CMake toolchain file for building TigerVNC for Apple Silicon (ARM64)
#
# This toolchain file is useful for:
# 1. Cross-compiling from x86_64 macOS to ARM64 macOS
# 2. Ensuring ARM64 builds on Apple Silicon (though native builds work too)
#
# Usage:
#   mkdir build-apple-arm64
#   cd build-apple-arm64
#   cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchains/apple-arm64.cmake ..
#   make

set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR arm64)

# Use the system clang compiler with ARM64 target
# On Apple Silicon, this is native; on Intel Mac, this cross-compiles
set(CMAKE_OSX_ARCHITECTURES arm64)

# Minimum macOS version that supports Apple Silicon
# macOS 11.0 (Big Sur) was the first version with Apple Silicon support
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

# Compiler flags for optimal ARM64 performance on Apple Silicon
# Apple Silicon supports ARMv8.4-A + additional features
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch arm64" CACHE STRING "C flags for Apple ARM64")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch arm64" CACHE STRING "CXX flags for Apple ARM64")

# Optional: Enable additional optimizations for Apple Silicon
# Uncomment if you want to specifically target M1/M2/M3 features
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=apple-m1")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=apple-m1")
