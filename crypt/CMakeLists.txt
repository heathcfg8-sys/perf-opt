project(Cryption)
cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
# 添加架构和调优选项
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a -mtune=cortex-a72")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
# 添加可执行文件
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_executable(md5_hash md5/md5.cpp md5/md5_main.cpp md5/md5_interface.cpp md5/md5_interface.h)
add_executable(sha256_hash sha256/sha256.cpp sha256/sha256_main.cpp sha256/sha256_interface.cpp sha256/sha256_interface.h)
add_executable(aes_encrypt aes/aes.cpp aes/gmult.cpp aes/aes_main.cpp
        param.h aes/aes_interface.cpp aes/aes_interface.h)

# 如果有头文件，添加包含目录
target_include_directories(md5_hash PRIVATE md5)
target_include_directories(sha256_hash PRIVATE sha256)
target_include_directories(aes_encrypt PRIVATE aes)
