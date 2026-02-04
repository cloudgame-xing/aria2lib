cmake_minimum_required(VERSION 3.20)

project(aria2_c_api_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(aria2_c_api SHARED
  src/aria2_c_api.cpp
)

target_compile_definitions(aria2_c_api PRIVATE ARIA2_C_API_BUILD)

target_include_directories(aria2_c_api PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/out/aria2/win-x64/include)
target_link_directories(aria2_c_api PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/out/aria2/win-x64/lib)
target_link_libraries(aria2_c_api PRIVATE aria2 gmp z cares ssh2 ssl crypto expat sqlite3 secur32 crypt32 wsock32 iphlpapi ws2_32)

add_executable(aria2_c_api_main
  src/main.cpp
)

target_include_directories(aria2_c_api_main PRIVATE src)
target_link_libraries(aria2_c_api_main PRIVATE aria2_c_api)

if(MINGW)
  target_link_options(aria2_c_api PRIVATE -static -static-libgcc -static-libstdc++)
  target_link_options(aria2_c_api_main PRIVATE -static -static-libgcc -static-libstdc++)
endif()
