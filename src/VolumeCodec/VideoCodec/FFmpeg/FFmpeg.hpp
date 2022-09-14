#pragma once
#include "../VideoCodec.hpp"
/**
 * @note for encode, data must copy to ffmpeg's owner buffer, so interface may use a lambda to get value...
 * for decode, just provide ptr and size is ok.
 */
using namespace vol;
class FFmpegCodecPrivate;
class FFmpegCodec{
public:
    FFmpegCodec();

    ~FFmpegCodec();

    bool IsValid() const;

    bool Reset(const SharedVideoCodecParams& params);

    void EncodeFrameIntoPackets(const void *buf, size_t size, Packets &packets);

    size_t DecodePacketIntoFrames(const Packet &packet, void *buf, size_t size);
private:
    std::unique_ptr<FFmpegCodecPrivate> _;
};