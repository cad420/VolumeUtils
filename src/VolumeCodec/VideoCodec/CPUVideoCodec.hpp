#pragma once

#include "VideoCodec.hpp"

VOL_BEGIN

class CPUVideoCodecImpl;
class CPUVideoCodec final : public VideoCodec{
public:
    CPUVideoCodec();

    ~CPUVideoCodec() override;

    bool ReSet(const CodecParams& params) override;

    void EncodeFrameIntoPackets(const void* buf, size_t size, Packets& packets) override;

    size_t DecodePacketIntoFrames(const Packet& packet, void* buf, size_t size) override;

private:
    std::unique_ptr<CPUVideoCodecImpl> _;
};


VOL_END