#pragma once

#include "VolumeUtils/Volume.hpp"

VOL_BEGIN

enum class VideoCodecFormat{
    NONE,
    YUV420_8BIT,
    YUV420_10BIT,
    YUV420_12BIT
};

struct HevcVideoCodecParams{
//    static constexpr int MAX_SAMPLES = 4;
//    uint8_t* data;//continuous chroma planes storage
//    std::vector<std::vector<uint8_t>> frame[MAX_SAMPLES];//separate chroma planes storage

    int frame_width;
    int frame_height;
    int frame_count;
    VideoCodecFormat fmt;
    bool is_encode;// true for encode and false for decode

};

/**
 * @brief Same voxel coded by gpu or cpu should get same result, so cpu and gpu should share codec params
 */

using SharedVideoCodecParams = HevcVideoCodecParams;

inline bool TransformToShared(const VideoCodec::CodecParams& params, CodecDevice device, SharedVideoCodecParams* ret){

    return true;
}

VOL_END
