file(GLOB Processor_SRCS "*.cpp")
add_executable(VolumeProcessor ${Processor_SRCS})
target_link_libraries(VolumeProcessor PRIVATE VolumeUtils)
target_compile_features(
        VolumeProcessor
        PRIVATE
        cxx_std_20
)