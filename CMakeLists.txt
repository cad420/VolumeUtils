cmake_minimum_required(VERSION 3.16)
project(VolumeUtils)

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

set(
    LIBTIFF_LIBS
        ${PROJECT_SOURCE_DIR}/deps/binary/libtiff/lib/x64/tiff.lib
        ${PROJECT_SOURCE_DIR}/deps/binary/libtiff/lib/x64/tiffd.lib
        ${PROJECT_SOURCE_DIR}/deps/binary/libtiff/lib/x64/tiffxx.lib
        ${PROJECT_SOURCE_DIR}/deps/binary/libtiff/lib/x64/tiffxxd.lib
)

set(
        FFMPEG_LIBS
        ${PROJECT_SOURCE_DIR}/deps/binary/ffmpeg/lib/x64/avcodec.lib
        ${PROJECT_SOURCE_DIR}/deps/binary/ffmpeg/lib/x64/avutil.lib
)

target_link_libraries(
        VolumeUtils
        PRIVATE
        ${LIBTIFF_LIBS}
        ${FFMPEG_LIBS}
)

add_subdirectory(test)

add_subdirectory(tool)