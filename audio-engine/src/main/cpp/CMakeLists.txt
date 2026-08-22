cmake_minimum_required(VERSION 3.22.1)

project("auravoice_audio")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -ffast-math")

add_library(
    auravoice_audio
    SHARED
    NativeAudioEngine.cpp
)

target_compile_features(
    auravoice_audio
    PRIVATE
    cxx_std_20
)

target_compile_options(
    auravoice_audio
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

find_library(
    log-lib
    log
)

find_library(
    android-lib
    android
)

target_link_libraries(
    auravoice_audio
    ${log-lib}
    ${android-lib}
)
