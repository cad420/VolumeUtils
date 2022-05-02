add_executable(TestGridVolume TestGridVolume.cpp)
target_link_libraries(TestGridVolume PRIVATE VolumeUtils)
target_compile_features(
        TestGridVolume
        PRIVATE
        cxx_std_17
)