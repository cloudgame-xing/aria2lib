cmake_minimum_required(VERSION 3.20)

project(aria2_c_api_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(aria2_c_api SHARED
  src/aria2_c_api.cpp
)

target_compile_definitions(aria2_c_api PRIVATE ARIA2_C_API_BUILD)

add_executable(aria2_c_api_main
  src/main.cpp
)

target_include_directories(aria2_c_api_main PRIVATE src)
target_link_libraries(aria2_c_api_main PRIVATE aria2_c_api)

if(MINGW)
  target_include_directories(aria2_c_api PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/out/aria2/include)
  target_link_directories(aria2_c_api PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/out/aria2/lib)
  target_link_libraries(aria2_c_api PRIVATE aria2 gmp z cares ssh2 ssl crypto expat sqlite3 secur32 crypt32 wsock32 iphlpapi ws2_32)

  target_link_options(aria2_c_api PRIVATE -static -static-libgcc -static-libstdc++)
  target_link_options(aria2_c_api_main PRIVATE -static -static-libgcc -static-libstdc++)
elseif(LINUX)
  target_include_directories(aria2_c_api PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/out/aria2/include)
  target_link_directories(aria2_c_api PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/out/aria2/lib)

  target_link_options(aria2_c_api PRIVATE
    -static-libstdc++
    -static-libgcc
  )

  target_link_libraries(aria2_c_api PRIVATE
    -Wl,-Bstatic
    aria2 gmp z cares ssh2 ssl crypto expat sqlite3
    -Wl,-Bdynamic
    c m dl pthread rt
  )

  target_link_options(aria2_c_api PRIVATE
    "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/version.script"
  )

  # target_link_options(aria2_c_api_main PRIVATE -static -static-libgcc -static-libstdc++)
elseif(APPLE)
  execute_process(
    COMMAND brew --prefix
    OUTPUT_VARIABLE HOMEBREW_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  target_include_directories(aria2_c_api PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/out/aria2/include)
  target_link_directories(aria2_c_api PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/out/aria2/lib)
  target_link_libraries(aria2_c_api PRIVATE libaria2.a libz.a libxml2.a sqlite3
    "${HOMEBREW_PREFIX}/opt/c-ares/lib/libcares.a"
    "${HOMEBREW_PREFIX}/opt/libssh2/lib/libssh2.a"
    "${HOMEBREW_PREFIX}/opt/openssl/lib/libssl.a"
    "${HOMEBREW_PREFIX}/opt/openssl/lib/libcrypto.a"
  )

endif()

install(TARGETS aria2_c_api
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

install(FILES src/aria2_c_api.h DESTINATION include)

install(TARGETS aria2_c_api_main
  RUNTIME DESTINATION bin
)
