add_executable(VolumeProcessor VolumeProcessor.cpp)
target_link_libraries(VolumeProcessor PRIVATE VolumeUtils)
target_compile_features(
        VolumeProcessor
        PRIVATE
        cxx_std_17
)