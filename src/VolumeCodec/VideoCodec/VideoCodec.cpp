#include "CPUVideoCodec.hpp"
#include "GPUVideoCodec.hpp"

VOL_BEGIN


std::unique_ptr<VideoCodec> VideoCodec::Create(CodecDevice device) {
    if(device == CodecDevice::CPU)
        return std::make_unique<CPUVideoCodec>();
    else if(device == CodecDevice::GPU)
        return std::make_unique<GPUVideoCodec>();
    return nullptr;
}

VOL_END