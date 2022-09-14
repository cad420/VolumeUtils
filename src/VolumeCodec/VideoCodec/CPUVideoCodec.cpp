#include "CPUVideoCodec.hpp"
#include "FFmpeg/FFmpeg.hpp"

VOL_BEGIN
class CPUVideoCodecImpl{
public:
    FFmpegCodec codec;
};

CPUVideoCodec::CPUVideoCodec() {
    _ = std::make_unique<CPUVideoCodecImpl>();

}

CPUVideoCodec::~CPUVideoCodec() {

}

bool CPUVideoCodec::ReSet(const VideoCodec::CodecParams &params) {
    assert(_->codec.IsValid());
    SharedVideoCodecParams ret;
    auto ok = TransformToShared(params, CodecDevice::CPU, &ret);
    if(!ok) return false;
    return _->codec.Reset(ret);
}

void CPUVideoCodec::EncodeFrameIntoPackets(const void *buf, size_t size, Packets &packets) {
    assert(_->codec.IsValid());
    _->codec.EncodeFrameIntoPackets(buf, size, packets);
}

size_t CPUVideoCodec::DecodePacketIntoFrames(const Packet &packet, void *buf, size_t size) {
    assert(_->codec.IsValid());
    return _->codec.DecodePacketIntoFrames(packet, buf, size);
}


VOL_END

