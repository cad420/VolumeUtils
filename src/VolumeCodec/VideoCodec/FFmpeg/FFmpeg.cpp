#include "FFmpeg.hpp"
#include "Common/Utils.hpp"

using namespace vol;

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

namespace{
    size_t AV__DecodePacketIntoFrames(AVCodecContext* c, AVFrame* frame, AVPacket* pkt, uint8_t* buf){
        int ret = avcodec_send_packet(c, pkt);
        if(ret < 0){
            throw VideoCodecError("AVDecode error: send packet failed with error " + std::to_string(ret));
        }

        size_t size = 0;
        while(ret >= 0){
            ret = avcodec_receive_frame(c, frame);
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            else if(ret < 0)
                throw VideoCodecError("AVDecode error: receive frame failed with error " + std::to_string(ret));

            size_t frame_size = frame->linesize[0] * frame->height;
            std::memcpy(buf + size, frame->data[0], frame_size);
            size += frame_size;
        }
        return size;
    }

    void AV__EncodeFrameIntoPackets(AVCodecContext* c, AVFrame* frame, AVPacket* pkt, Packets& packets){
        int ret = avcodec_send_frame(c, frame);
        if(ret < 0){
            throw VideoCodecError("AVEncode error: send frame failed with error " + std::to_string(ret));
        }

        while(ret >= 0){
            ret = avcodec_receive_packet(c, pkt);
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            else if(ret < 0)
                throw VideoCodecError("AVEncode error: receive packet failed with error " + std::to_string(ret));

            // just copy one channel data for gray now
            auto& packet = packets.emplace_back(pkt->size);
            std::memcpy(packet.data(), pkt->data, pkt->size);

            av_packet_unref(pkt);
        }
    }

    AVPixelFormat TransformPixelFormat(VideoCodecFormat fmt){
        assert(fmt != vol::VideoCodecFormat::NONE);
        switch (fmt) {
            case vol::VideoCodecFormat::YUV420_8BIT : return AV_PIX_FMT_YUV420P;
            case vol::VideoCodecFormat::YUV420_10BIT : return AV_PIX_FMT_YUV420P10;
            case vol::VideoCodecFormat::YUV420_12BIT : return AV_PIX_FMT_YUV420P12;
            default : return AV_PIX_FMT_NONE;
        }
    }
}

class FFmpegCodecPrivate{
public:
    enum ContextState{
        INVALID,
        ENCODE,
        DECODE
    };
    ContextState state;

    const AVCodec* codec = nullptr;
    AVCodecContext* ctx = nullptr;
    AVPacket* pkt = nullptr;
    AVFrame* frame = nullptr;

    int64_t pts = 0;

    void FreeContext(){
        state = INVALID;
        codec = nullptr;
        if(ctx)
            avcodec_free_context(&ctx);
        if(pkt)
            av_packet_free(&pkt);
        if(frame)
            av_frame_free(&frame);
        pts = 0;
    }
    static void my_log_callback(void *ptr, int level, const char *fmt, va_list vargs)
    {
//        vprintf(fmt, vargs);
    }
    bool InitEncodeContext(const SharedVideoCodecParams& params){
        FreeContext();
        av_log_set_level(AV_LOG_QUIET);
        av_log_set_callback(my_log_callback);
        if(params.fmt == vol::VideoCodecFormat::NONE){
            std::cerr << "Invalid format NONE" << std::endl;
            return false;
        }
        ScopeGuard guard([this]{
            FreeContext();
        });
        codec = avcodec_find_encoder(AV_CODEC_ID_HEVC);
        if(!codec){
            std::cerr << "AVCodec error: not find codec hevc" << std::endl;
            return false;
        }

        ctx = avcodec_alloc_context3(codec);
        if(!ctx){
            std::cerr << "AVCodec error: alloc context failed" << std::endl;
            return false;
        }

        pkt = av_packet_alloc();
        if(!pkt){
            std::cerr << "AVCodec error: alloc packet failed" << std::endl;
            return false;
        }

        frame = av_frame_alloc();
        if(!frame){
            std::cerr << "AVCodec error: alloc frame falied " << std::endl;
            return false;
        }

        // set avcodec context params
        ctx->width = params.frame_width;
        ctx->height = params.frame_height;
        ctx->time_base = {1, 30};
        ctx->framerate = {30, 1};
        ctx->bits_per_raw_sample = params.bits_per_sampler;
        ctx->pix_fmt = TransformPixelFormat(params.fmt);
        // because interfaces in Volume.hpp not consider of encode quality... just set medium
        av_opt_set(ctx->priv_data, "preset", "medium", 0);
        av_opt_set(ctx->priv_data, "tune", "fastdecode", 0);
        int ret = avcodec_open2(ctx, codec, nullptr);
        if(ret < 0){
            std::cerr << "AVCodec error: open codec failed" << std::endl;
            return false;
        }

        frame->format = ctx->pix_fmt;
        frame->width = ctx->width;
        frame->height = ctx->height;
        ret = av_frame_get_buffer(frame, 0);
        if(ret < 0){
            std::cerr << "AVCodec error: frame get buffer failed" << std::endl;
            return false;
        }

        state = ENCODE;
        guard.Dismiss();
        return true;
    }

    bool InitDecodeContext(const SharedVideoCodecParams& params){
        FreeContext();
        av_log_set_level(AV_LOG_ERROR);
        if(params.fmt == vol::VideoCodecFormat::NONE){
            std::cerr << "Invalid format NONE" << std::endl;
            return false;
        }
        ScopeGuard guard([this]{
            FreeContext();
        });
        codec = avcodec_find_encoder(AV_CODEC_ID_HEVC);
        if(!codec){
            std::cerr << "AVCodec error: not find codec hevc" << std::endl;
            return false;
        }

        ctx = avcodec_alloc_context3(codec);
        if(!ctx){
            std::cerr << "AVCodec error: alloc context failed" << std::endl;
            return false;
        }

        pkt = av_packet_alloc();
        if(!pkt){
            std::cerr << "AVCodec error: alloc packet failed" << std::endl;
            return false;
        }

        frame = av_frame_alloc();
        if(!frame){
            std::cerr << "AVCodec error: alloc frame falied " << std::endl;
            return false;
        }

        ctx->delay = 0;
        ctx->thread_count = params.threads_count;
        int ret = avcodec_open2(ctx, codec, nullptr);
        if(ret < 0){
            std::cerr << "AVCodec error: open codec failed" << std::endl;
            return false;
        }

        guard.Dismiss();

        state = DECODE;
        return true;
    }

    bool IsContextValid() const{
        return state != INVALID;
    }
};

FFmpegCodec::FFmpegCodec() {
    _ = std::make_unique<FFmpegCodecPrivate>();
}

FFmpegCodec::~FFmpegCodec() {
    _->FreeContext();
}

bool FFmpegCodec::Reset(const SharedVideoCodecParams &params) {
    if(params.is_encode)
        return _->InitEncodeContext(params);
    else
        return _->InitDecodeContext(params);
}

void FFmpegCodec::EncodeFrameIntoPackets(const void *buf, size_t size, Packets &packets) {
    // check context
    assert(_->state == FFmpegCodecPrivate::ENCODE);
//    assert(buf && size);// nullptr for end

    // copy buf to frame
    int ret = av_frame_make_writable(_->frame);
    if(ret < 0)
        throw VideoCodecError("AVEncode error: frame make writable failed with error " + std::to_string(ret));

    // only copy buf to data 0 which represents one channel for gray/Y(YUV)
    if(buf){
        //_->frame->data[0] = reinterpret_cast<uint8_t*>(const_cast<void*>(buf));// not ok because of other channel data
        std::memcpy(_->frame->data[0], buf, size);
        _->frame->pts = _->pts++;
    }
    // encode into packets
    AV__EncodeFrameIntoPackets(_->ctx, buf ? _->frame : nullptr, _->pkt, packets);
}

size_t FFmpegCodec::DecodePacketIntoFrames(const Packet &packet, void *buf, size_t size) {
    assert(_->state == FFmpegCodecPrivate::DECODE);
    assert(buf && size);

    int ret = av_packet_make_writable(_->pkt);
    if(ret < 0)
        throw VideoCodecError("AVDecode error: packet make writable failed with error " + std::to_string(ret));

    _->pkt->data = reinterpret_cast<uint8_t*>(buf);
    _->pkt->size = size;

    return AV__DecodePacketIntoFrames(_->ctx, _->frame, _->pkt, reinterpret_cast<uint8_t*>(buf));
}

bool FFmpegCodec::IsValid() const {
    return _->IsContextValid();
}


