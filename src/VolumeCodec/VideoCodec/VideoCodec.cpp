#include "CPUVideoCodec.hpp"
#include "GPUVideoCodec.hpp"

VOL_BEGIN


std::unique_ptr<VideoCodec> VideoCodec::Create(CodecDevice device) {
    return nullptr;
}

VOL_END