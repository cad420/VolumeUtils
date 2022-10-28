cmake_minimum_required(VERSION 3.16)
project(VolumeUtils)

option(ENABLE_HIGH_PERFORMANCE "" ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

file(
        GLOB_RECURSE
        SRCS
        src/*.cpp
)

add_library(VolumeUtils ${SRCS})
target_include_directories(
        VolumeUtils
        PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/deps/source
        ${PROJECT_SOURCE_DIR}/deps/binary/libtiff/include
        ${PROJECT_SOURCE_DIR}/deps/binary/ffmpeg/include
        ${PROJECT_SOURCE_DIR}/deps/binary/nvcodec/include
        PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)
target_compile_features(
        VolumeUtils
        PUBLIC
        cxx_std_20
)

if(ENABLE_HIGH_PERFORMANCE)
    target_compile_definitions(VolumeUtils PRIVATE HIGH_PERFORMANCE)
endif()

set(
    LIBTIFF_LIBS
        ${PROJECT_SOURCE_DIR}/deps/binary/libtiff/lib/x64/tiff.lib
        ${PROJECT_SOURCE_DIR}/deps/binary/libtiff/lib/x64/tiffd.lib
        ${PROJECT_SOURCE_DIR}/deps/binary/libtiff/lib/x64/tiffxx.lib
        ${PROJECT_SOURCE_DIR}/deps/binary/libtiff/lib/x64/tiffxxd.lib
)

set(LIBTIFF_BINS
        ${PROJECT_SOURCE_DIR}/deps/binary/libtiff/bin/x64/tiff.dll
        ${PROJECT_SOURCE_DIR}/deps/binary/libtiff/bin/x64/tiffd.dll
        ${PROJECT_SOURCE_DIR}/deps/binary/libtiff/bin/x64/tiffxx.dll
        ${PROJECT_SOURCE_DIR}/deps/binary/libtiff/bin/x64/tiffxxd.dll
        )
file(COPY ${LIBTIFF_BINS} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

set(
        FFMPEG_LIBS
        ${PROJECT_SOURCE_DIR}/deps/binary/ffmpeg/lib/x64/avcodec.lib
        ${PROJECT_SOURCE_DIR}/deps/binary/ffmpeg/lib/x64/avutil.lib
)

set(FFMPEG_BINS
        ${PROJECT_SOURCE_DIR}/deps/binary/ffmpeg/bin/x64/avcodec-59.dll
        ${PROJECT_SOURCE_DIR}/deps/binary/ffmpeg/bin/x64/avutil-57.dll)

file(COPY ${FFMPEG_BINS} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

target_link_libraries(
        VolumeUtils
        PRIVATE
        ${LIBTIFF_LIBS}
        ${FFMPEG_LIBS}
)



add_subdirectory(test)

add_subdirectory(tool)