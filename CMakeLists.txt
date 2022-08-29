cmake_minimum_required(VERSION 3.16)
project(VolumeUtils)

file(
        GLOB
        SRCS
        src/*.cpp
)

add_library(VolumeUtils ${SRCS})
target_include_directories(
        VolumeUtils
        PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)
target_compile_features(
        VolumeUtils
        PUBLIC
        cxx_std_20
)

add_subdirectory(test)