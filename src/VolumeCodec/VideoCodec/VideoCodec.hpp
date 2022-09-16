#pragma once

#include "VolumeUtils/Volume.hpp"

#include <iostream>

VOL_BEGIN

class VideoCodecError : public std::exception{
public:
    VideoCodecError(const std::string& msg) : std::exception(msg.c_str()){}
};

enum class VideoCodecFormat{
    NONE,
    YUV420_8BIT,
    YUV420_10BIT,
    YUV420_12BIT
};

struct HevcVideoCodecParams{
    int frame_width;
    int frame_height;
    int frame_count;
    VideoCodecFormat fmt;
    bool is_encode;// true for encode and false for decode

    int bits_per_sampler;
    int threads_count;
};

/**
 * @brief Same voxel coded by gpu or cpu should get same result, so cpu and gpu should share codec params
 */

using SharedVideoCodecParams = HevcVideoCodecParams;

/**
 * @note current only support gray 8 and 16.
 * RGB and RGBA format is also ok but should handle rgb -> yuv and yuv -> rgb,
 * this may be added in later...
 */
inline bool TransformToShared(const VideoCodec::CodecParams& params, CodecDevice device, SharedVideoCodecParams* ret){
    if(params.frame_w == 0 || params.frame_w % 2
    || params.frame_h == 0 || params.frame_h % 2){
        std::cerr << "Error: Invalid frame shape" << std::endl;
        return false;
    }
    if(params.samplers_per_pixel != 1){
        std::cerr << "Error: Only support gray image to encode" << std::endl;
        return false;
    }
    if(params.bits_per_sampler != 8 && params.bits_per_sampler != 16){
        std::cerr << "Error: Only support 8 or 16 bits per raw sampler" << std::endl;
        return false;
    }
    if(ret){
        ret->frame_width = params.frame_w;
        ret->frame_height = params.frame_h;
        ret->frame_count = params.frame_n;
        ret->threads_count = params.threads_count;
        ret->bits_per_sampler = params.bits_per_sampler;
        ret->is_encode = params.encode;
        if(params.bits_per_sampler == 8){
            ret->fmt = VideoCodecFormat::YUV420_8BIT;
        }
        else if(params.bits_per_sampler == 16){
            ret->fmt = device == CodecDevice::CPU ? VideoCodecFormat::YUV420_12BIT : VideoCodecFormat::YUV420_10BIT;
        }
    }
    return true;
}

VOL_END
