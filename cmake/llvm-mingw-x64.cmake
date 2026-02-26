# Cross-compile for Windows x64 using llvm-mingw on Linux.
# Requires environment variable LLVM_MINGW to point to the llvm-mingw root
# (e.g. /path/to/llvm-mingw-20260224-msvcrt-ubuntu-22.04-x86_64).

set(LLVM_MINGW "$ENV{LLVM_MINGW}")
if(NOT LLVM_MINGW)
  message(FATAL_ERROR "LLVM_MINGW environment variable is not set")
endif()

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER "${LLVM_MINGW}/bin/x86_64-w64-mingw32-clang")
set(CMAKE_CXX_COMPILER "${LLVM_MINGW}/bin/x86_64-w64-mingw32-clang++")
set(CMAKE_RC_COMPILER "${LLVM_MINGW}/bin/x86_64-w64-mingw32-windres")
set(CMAKE_AR "${LLVM_MINGW}/bin/llvm-ar" CACHE FILEPATH "Archiver")
set(CMAKE_RANLIB "${LLVM_MINGW}/bin/llvm-ranlib" CACHE FILEPATH "Ranlib")
set(CMAKE_STRIP "${LLVM_MINGW}/bin/llvm-strip" CACHE FILEPATH "Strip")

set(CMAKE_FIND_ROOT_PATH "${LLVM_MINGW}/x86_64-w64-mingw32" "${CMAKE_FIND_ROOT_PATH}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Mark cross-compile so CMakeLists.txt can add build/deps/out for static libs
set(ARIA2_WINDOWS_X64_CROSS 1 CACHE INTERNAL "")
