#include "GPUVideoCodec.hpp"
#include "NVCodec/NVCodec.hpp"
VOL_BEGIN
class GPUVideoCodecImpl{
public:

};
GPUVideoCodec::GPUVideoCodec() {

}

GPUVideoCodec::~GPUVideoCodec() {

}

bool GPUVideoCodec::ReSet(const VideoCodec::CodecParams &params) {
    return false;
}

void GPUVideoCodec::EncodeFrameIntoPackets(const void *buf, size_t size, Packets &packets) {

}

size_t GPUVideoCodec::DecodePacketIntoFrames(const Packet &packet, void *buf, size_t size) {
    return 0;
}

VOL_END
