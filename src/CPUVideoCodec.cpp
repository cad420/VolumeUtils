//
// Created by wyz on 2022/5/11.
//
#include <VolumeUtils/Volume.hpp>
#include "VideoCodec.hpp"
#include <iostream>
bool EncodeBits(const Extend3D &shape, const void *buf, Packets &packets, int bitsPerSample, int nSamples, int maxThreads) {
    HevcVideoCodecParam codec_param;
    codec_param.data = nullptr;
    codec_param.width = shape.width;
    codec_param.height = shape.height;
    codec_param.frames = shape.depth;
    codec_param.frame_size = codec_param.width * codec_param.height * bitsPerSample * nSamples / 8;
    if(nSamples == 1){
        codec_param.frame[0].resize(codec_param.frames);
        for(auto& frame:codec_param.frame[0]){
            frame.resize(codec_param.frame_size);
            std::copy(static_cast<uint8_t*>(const_cast<void*>(buf)),
                      static_cast<uint8_t*>(const_cast<void*>(buf))+codec_param.frame_size,
                      frame.data());
        }
    }
    else{
        std::cerr<<"not support nSamples > 1 at current!"<<std::endl;
        return false;
    }
}

bool DecodeBits(const Packets &packets, void *buf, int bitsPerSample, int nSamples, int maxThreads) {
    return false;
}


template<typename T>
bool VideoCodec<T, CodecDevice::CPU, std::enable_if_t<VideoCodecVoxelV<T>>>::Encode(
        const std::vector<SliceDataView<T>> &slices, Packet &packet, int maxThreads) {
    return false;
}

template<typename T>
bool VideoCodec<T, CodecDevice::CPU, std::enable_if_t<VideoCodecVoxelV<T>>>::Encode(const GridDataView<T> &volume,
                                                                                    SliceAxis axis, Packet &packet,
                                                                                    int maxThreads) {
    return false;
}

template<typename T>
bool VideoCodec<T, CodecDevice::CPU, std::enable_if_t<VideoCodecVoxelV<T>>>::Encode(
        const std::vector<SliceDataView<T>> &slices, Packets &packets, int maxThreads) {
    return false;
}

template<typename T>
bool VideoCodec<T, CodecDevice::CPU, std::enable_if_t<VideoCodecVoxelV<T>>>::Encode(const GridDataView<T> &volume,
                                                                                    SliceAxis axis, Packets &packets,
                                                                                    int maxThreads) {
    return false;
}

template<typename T>
bool VideoCodec<T, CodecDevice::CPU, std::enable_if_t<VideoCodecVoxelV<T>>>::Decode(const Packet &packet, T *buf,
                                                                                    int maxThreads) {
    return false;
}

template<typename T>
bool VideoCodec<T, CodecDevice::CPU, std::enable_if_t<VideoCodecVoxelV<T>>>::Decode(const Packets &packets, T *buf,
                                                                                    int maxThreads) {
    return false;
}
