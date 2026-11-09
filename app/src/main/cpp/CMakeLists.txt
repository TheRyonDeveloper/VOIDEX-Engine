cmake_minimum_required(VERSION 3.22.1)

project(VOIDEX_ENGINE LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)

add_library(
    voidex
    SHARED
    native-lib.cpp
)

find_library(
    GLES_LIBRARY
    GLESv3
)

find_library(
    LOG_LIBRARY
    log
)

find_library(
    ANDROID_LIBRARY
    android
)

target_compile_options(
    voidex
    PRIVATE
    -Wall
    -Wextra
    -O2
)

target_link_libraries(
    voidex
    ${GLES_LIBRARY}
    ${LOG_LIBRARY}
    ${ANDROID_LIBRARY}
)
