
option(SLICE_FORMAT_IS_TIF "" ON)

file(GLOB_RECURSE Processor_SRCS "*.cpp")
add_executable(VolumeProcessor ${Processor_SRCS})
target_link_libraries(VolumeProcessor PRIVATE VolumeUtils)
target_include_directories(VolumeProcessor PRIVATE ${PROJECT_SOURCE_DIR}/src/Common)
target_compile_features(
        VolumeProcessor
        PRIVATE
        cxx_std_20
)

if(SLICE_FORMAT_IS_TIF)
target_compile_definitions(VolumeProcessor PRIVATE SLICE_FILE_NO_RANDOM_ACCESS)
endif()