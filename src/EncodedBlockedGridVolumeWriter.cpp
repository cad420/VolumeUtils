//
// Created by wyz on 2022/5/2.
//
#include <VolumeUtils/Volume.hpp>
#include "EncodedBlockedGridVolumeFile.hpp"
#include <iostream>
#include <fstream>
#include <cassert>

VOL_BEGIN

class EncodedBlockedGridVolumeWriterPrivate{
public:
    EncodedBlockedGridVolumeFile file;
    std::ofstream out;
    size_t WritePackets(const Packets& packets){
        size_t write_size = 0;
        for(auto& packet:packets){
            size_t packet_size = packet.size();
            out.write(reinterpret_cast<char*>(&packet_size),sizeof(packet_size));
            write_size += sizeof(packet_size);
            out.write(reinterpret_cast<const char*>(packet.data()),packet.size());
            write_size += packet_size;
        }
        return write_size;
    }
    void WriteBlockInfoMap(){
        for(const auto& item:file.mp){
            const auto& blockInfo = item.second;
            out.write(reinterpret_cast<const char*>(&blockInfo),sizeof(blockInfo));
        }
    }
    void WriteHeader(){
        out.write(reinterpret_cast<char*>(&file.header),sizeof(file.header));
    }
};

EncodedBlockedGridVolumeWriter::EncodedBlockedGridVolumeWriter(const std::string &filename,
                                                               const EncodedBlockedGridVolumeDesc &desc) {
    _->out.open(filename,std::ios::binary|std::ios::beg);
    if(!_->out.is_open()){
        throw std::runtime_error("failed to open file: "+filename);
    }
    if(!CheckValidation(desc)){
        throw std::runtime_error("invalid encoded blocked grid volume desc");
    }
    _->file.header.desc = desc;
}

EncodedBlockedGridVolumeWriter::~EncodedBlockedGridVolumeWriter() {
    if(_->out.is_open()){
        Close();
    }
}

const EncodedBlockedGridVolumeDesc &EncodedBlockedGridVolumeWriter::GetEncodedBlockedGridVolumeDesc() const {
    return _->file.header.desc;
}

void EncodedBlockedGridVolumeWriter::WriteBlockData(const BlockIndex &blockIndex, const void *buf, EncodeWorker worker) {
    if(worker == nullptr){
        worker = [this](const void* buf,Packets& packets){
            bool e = EncodeBits(
                    {_->file.header.desc.block_length,
                     _->file.header.desc.block_length,
                     _->file.header.desc.block_length},
                    buf,packets,
                       GetVoxelBits(_->file.header.desc.type),
                    GetVoxelSampleCount(_->file.header.desc.format));
            return e;
        };
    }
    Packets packets;
    bool e = worker(buf,packets);
    if(!e){
        std::cerr<<"encode block failed: "<<blockIndex.x<<" "<<blockIndex.y<<" "<<blockIndex.z<<std::endl;
        return;
    }
    WriteRawBlockData(blockIndex,packets);
}

void EncodedBlockedGridVolumeWriter::WriteRawBlockData(const BlockIndex &blockIndex, const Packet &packet) {
    //use ffmpeg av_parser
    std::cerr<<"this method is not implied at current!"<<std::endl;
}

void EncodedBlockedGridVolumeWriter::WriteRawBlockData(const BlockIndex &blockIndex, const Packets &packets) {
    assert(!packets.empty());
    if(_->file.mp.count(blockIndex) == 1){
        std::cerr<<"error: write raw block data which has written already"<<std::endl;
        return;
    }
    _->file.mp[blockIndex].index = blockIndex;
    _->file.mp[blockIndex].offset = _->out.tellp();
    _->file.mp[blockIndex].size = _->WritePackets(packets);
    _->file.mp[blockIndex].packet_count = packets.size();
    _->file.header.block_info_count += 1;
}

void EncodedBlockedGridVolumeWriter::Close() {
    if(!_->out.is_open()){
        throw std::runtime_error("close not open file");
    }
    _->file.header.block_info_size = _->file.header.block_info_count * EncodedBlockedGridVolumeFile::BlockInfoSize;
    _->file.header.block_info_offset = _->out.tellp();
    _->WriteBlockInfoMap();
    _->WriteHeader();
    _->out.close();
}

VOL_END