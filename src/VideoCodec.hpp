//
// Created by wyz on 2022/5/11.
//

#ifndef VOLUMEUTILS_VIDEOCODEC_HPP
#define VOLUMEUTILS_VIDEOCODEC_HPP

#include <VolumeUtils/Volume.hpp>
#include <vector>
struct HevcVideoCodecParam{
    static constexpr int MAX_SAMPLES = 4;
    enum CodecFormat{
        YUV420_8BIT,
        YUV420_10BIT,
        YUV420_12BIT
    } fmt;
    uint8_t* data;//continuous chroma planes storage
    std::vector<std::vector<uint8_t>> frame[MAX_SAMPLES];//separate chroma planes storage
    int width,height;
    int frame_size;
    int frames;
    int bits_per_sample;

};

void Encode(const HevcVideoCodecParam& param, Packets& packets);

#endif //VOLUMEUTILS_VIDEOCODEC_HPP
