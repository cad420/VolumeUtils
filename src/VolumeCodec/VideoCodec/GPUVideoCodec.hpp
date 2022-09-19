#pragma once

#include "VideoCodec.hpp"

VOL_BEGIN

class GPUVideoCodecImpl;
class GPUVideoCodec final : public VideoCodec{
public:
    GPUVideoCodec();

    ~GPUVideoCodec() override;

    bool ReSet(const CodecParams& params) override;

    void EncodeFrameIntoPackets(const void* buf, size_t size, Packets& packets) override;

    size_t DecodePacketIntoFrames(const Packet& packet, void* buf, size_t size) override;

private:
    std::unique_ptr<GPUVideoCodecImpl> _;
};


VOL_END