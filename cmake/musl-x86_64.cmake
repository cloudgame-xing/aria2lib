# Build for Linux x86_64 using musl toolchain.
# Requires environment variable MUSL_TOOLCHAIN_DIR to point to the extracted
# musl toolchain root (e.g. build/musl-1.2.3-platform-x86_64-unknown-linux-gnu-target-x86_64-linux-musl).

set(MUSL_TOOLCHAIN "$ENV{MUSL_TOOLCHAIN_DIR}")
if(NOT MUSL_TOOLCHAIN)
  message(FATAL_ERROR "MUSL_TOOLCHAIN_DIR environment variable is not set")
endif()

set(MUSL_SYSROOT "${MUSL_TOOLCHAIN}/x86_64-linux-musl")

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Force use of musl sysroot only; on x86_64 glibc host the driver may still search /usr/lib.
set(CMAKE_SYSROOT "${MUSL_SYSROOT}")

# Explicit --sysroot so compiler and linker never see host glibc (CMAKE_SYSROOT alone can be ignored).
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --sysroot=${MUSL_SYSROOT}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --sysroot=${MUSL_SYSROOT}" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--sysroot=${MUSL_SYSROOT}" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--sysroot=${MUSL_SYSROOT}" CACHE STRING "" FORCE)

set(CMAKE_C_COMPILER "${MUSL_TOOLCHAIN}/bin/x86_64-linux-musl-gcc")
set(CMAKE_CXX_COMPILER "${MUSL_TOOLCHAIN}/bin/x86_64-linux-musl-g++")
set(CMAKE_AR "${MUSL_TOOLCHAIN}/bin/x86_64-linux-musl-ar" CACHE FILEPATH "Archiver")
set(CMAKE_RANLIB "${MUSL_TOOLCHAIN}/bin/x86_64-linux-musl-ranlib" CACHE FILEPATH "Ranlib")
set(CMAKE_STRIP "${MUSL_TOOLCHAIN}/bin/x86_64-linux-musl-strip" CACHE FILEPATH "Strip")

set(CMAKE_FIND_ROOT_PATH "${MUSL_SYSROOT}" "${CMAKE_FIND_ROOT_PATH}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Mark that we use static deps from build/deps/out (like cross build)
set(ARIA2_LINUX_MUSL_X64 1 CACHE INTERNAL "")
