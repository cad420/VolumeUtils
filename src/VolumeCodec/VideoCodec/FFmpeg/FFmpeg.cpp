#include "FFmpeg.hpp"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

namespace{
    size_t AV__DecodePacketIntoFrames(AVCodecContext* c, AVFrame* frame, AVPacket* pkt, uint8_t* buf){

    }
    void AV_EncodeFrameIntoPackets(AVCodecContext* c, AVFrame* frame, AVPacket* pkt, Packets& packets){

    }
}

class FFmpegCodecPrivate{
public:
    bool is_encode;

    const AVCodec* codec = nullptr;
    AVCodecContext* ctx = nullptr;
    AVPacket* pkt = nullptr;
    AVFrame* frame = nullptr;

    void FreeContext(){

    }
    bool InitEncodeContext(const SharedVideoCodecParams& params){
        FreeContext();


        return true;
    }
    bool InitDecodeContext(const SharedVideoCodecParams& params){
        FreeContext();


        return true;
    }

};

FFmpegCodec::FFmpegCodec() {

}
FFmpegCodec::~FFmpegCodec() {

}
bool FFmpegCodec::Reset(const SharedVideoCodecParams &params) {
    if(params.is_encode)
        return _->InitEncodeContext(params);
    else
        return _->InitDecodeContext(params);
}

void FFmpegCodec::EncodeFrameIntoPackets(const void *buf, size_t size, Packets &packets) {
    //check context
    assert(_->is_encode);


}

size_t FFmpegCodec::DecodePacketIntoFrames(const Packet &packet, void *buf, size_t size) {
    assert(!_->is_encode);

    return 0;
}

bool FFmpegCodec::IsValid() const {
    return false;
}


