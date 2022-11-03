add_executable(TestGridVolume TestGridVolume.cpp)
target_link_libraries(TestGridVolume PRIVATE VolumeUtils)
target_compile_features(
        TestGridVolume
        PRIVATE
        cxx_std_17
)

add_executable(TestFFmpeg TestFFmpeg.cpp)
target_compile_features(
        TestFFmpeg
        PRIVATE
        cxx_std_20
)
target_link_libraries(
        TestFFmpeg
        PRIVATE

        ${FFMPEG_LIBS}
)
target_include_directories(
        TestFFmpeg
        PUBLIC
        ${PROJECT_SOURCE_DIR}/deps/binary/ffmpeg/include

)