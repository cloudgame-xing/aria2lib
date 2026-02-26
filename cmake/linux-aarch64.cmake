# Cross-compile for Linux aarch64 on x86_64 host.
# Uses system-installed gcc-aarch64-linux-gnu / g++-aarch64-linux-gnu.
# Expects deps built under build/deps/out (set ARIA2_LINUX_ARM64_CROSS).

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
set(CMAKE_FIND_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/deps/out" "${CMAKE_FIND_ROOT_PATH}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(ARIA2_LINUX_ARM64_CROSS 1 CACHE INTERNAL "")
