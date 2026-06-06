cmake_minimum_required(VERSION 3.0)
project(Cryption)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a -mtune=cortex-a72")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_executable(sha256_hash sha256/sha256.cpp sha256/sha256_main.cpp sha256/sha256_interface.cpp sha256/timestamp.cpp)
target_include_directories(sha256_hash PRIVATE sha256)