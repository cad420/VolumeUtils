extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}
#include <iostream>
#include <string>
#include <vector>
using Packet = std::vector<uint8_t>;
using Packets = std::vector<Packet>;



int raw_x = 256;
int raw_y = 256;
int raw_z = 256;
void encode(AVCodecContext* c,AVFrame* frame,AVPacket* pkt,std::vector<std::vector<uint8_t>>& packets,size_t& packets_size){
    int ret = avcodec_send_frame(c,frame);

    if(ret < 0){
        std::cerr<<"error sending a frame for encoding"<<std::endl;
        exit(1);
    }

    while(ret >= 0){
        ret = avcodec_receive_packet(c,pkt);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            break;
        }
        else if(ret < 0){
            std::cerr<<"error encoding"<<std::endl;
            exit(1);
        }
        packets_size += pkt->size;
        auto& packet = packets.emplace_back(pkt->size);
        std::memcpy(packet.data(), pkt->data, pkt->size);
        av_packet_unref(pkt);
    }
}
#define VoxelT uint8_t
void Encode(std::vector<VoxelT>& volume,std::vector<std::vector<uint8_t>>& packets,size_t packets_size){
    const auto codec = avcodec_find_encoder(AV_CODEC_ID_HEVC);
    if(!codec){
        std::cerr<<"not find codec"<<std::endl;
        exit(1);
    }
    AVCodecContext* c = avcodec_alloc_context3(codec);
    if(!c){
        std::cerr<<"alloc video codec context failed"<<std::endl;
        exit(1);
    }
    AVPacket* pkt = av_packet_alloc();
    if(!pkt){
        exit(1);
    }
//    c->bit_rate = 400000;
    c->width = raw_x;
    c->height = raw_y;
    c->time_base = {1,30};
    c->framerate = {30,1};
//    c->gop_size = 11;
    c->max_b_frames = 1;
    c->bits_per_raw_sample = sizeof(VoxelT) * 8;
//    c->level = 3;
//    c->thread_count = 10;
    c->pix_fmt = AV_PIX_FMT_YUV420P;//AV_PIX_FMT_YUV420P12;//AV_PIX_FMT_YUV420P12LE
//    c->log_level_offset = AV_LOG_QUIET;
    av_opt_set(c->priv_data,"preset","medium",0);
    av_opt_set(c->priv_data,"tune","fastdecode",0);
    av_opt_set(c->priv_data, "x265-params","log-level = 0",0);
    int ret = avcodec_open2(c,codec,nullptr);
    if(ret<0){
        std::cerr<<"open codec failed"<<std::endl;
        exit(1);
    }
    AVFrame* frame = av_frame_alloc();
    if(!frame){
        std::cerr<<"frame alloc failed"<<std::endl;
        exit(1);
    }
    frame->format = c->pix_fmt;
    frame->width = c->width;
    frame->height = c->height;
    ret = av_frame_get_buffer(frame,0);
    std::cout<<"linesize "<<frame->linesize[0]<<std::endl;
    std::cout<<"linesize "<<frame->linesize[1]<<std::endl;
    std::cout<<"linesize "<<frame->linesize[2]<<std::endl;
    std::cout<<"linesize "<<frame->linesize[3]<<std::endl;
    if(ret<0){
        std::cerr<<"could not alloc frame data"<<std::endl;
        exit(1);
    }
    //    LeftShiftArrayOfUInt16(volume.data(),voxel_count,6);
    for(int i = 0;i<raw_z;i++){
        ret = av_frame_make_writable(frame);

        if(ret<0){
            std::cerr<<"can't make frame write"<<std::endl;
            exit(1);
        }

        VoxelT * p = (VoxelT*)frame->data[0];

//        for(int y = 0;y<raw_y;y++){
//            for(int x = 0;x<raw_x;x++){
//                p[y * raw_y + x] = volume[i * raw_x * raw_y + y * raw_y + x];
//            }
//        }
        std::memcpy(p, volume.data() + i * raw_x * raw_y, raw_x * raw_y);
        frame->pts = i;

        encode(c,frame,pkt,packets,packets_size);
    }
    encode(c,nullptr,pkt,packets,packets_size);

    std::cout<<"packets count: "<<packets.size()<<" , size: "<<packets_size<<std::endl;
    avcodec_free_context(&c);
    av_frame_free(&frame);
    av_packet_free(&pkt);
}

void test(){
    for(int i = 0; i < 6; i++){
//        FFmpegCodec codec;
//        codec.Reset();
        size_t slice_size = raw_x * raw_y * raw_z;
        std::vector<uint8_t> slice(slice_size, 2);
//        size_t packed_size = 0;
//        for(int i = 0; i < 65; i++){
//            Packets tmp;
//            if(i < 64)
//                codec.EncodeFrameIntoPackets(slice.data(), slice_size, tmp);
//            else
//                codec.EncodeFrameIntoPackets(nullptr, 0, tmp);
//            for(auto& p : tmp) packed_size += p.size();
//        }
//        std::cout << "packed size : " << packed_size << std::endl;
//        codec._->FreeContext();
        Packets pkts;
        Encode(slice, pkts, 0);
        size_t packed_size = 0;
        for(auto& p : pkts) packed_size += p.size();
        std::cout << "packed size : " << packed_size << std::endl;
    }
}

int main(){


}


