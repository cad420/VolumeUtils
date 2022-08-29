//
// Created by wyz on 2022/5/2.
//
#include <VolumeUtils/Volume.hpp>
#include "EncodedBlockedGridVolumeFile.hpp"
#include <iostream>
#include <fstream>
#include <cassert>
#include <vector>

VOL_BEGIN

class EncodedBlockedGridVolumeReaderPrivate{
public:
    EncodedBlockedGridVolumeFile file;
    std::fstream in;
    void ReadHeader(){
        in.seekg(- static_cast<long long>(EncodedBlockedGridVolumeFile::HeaderSize),std::ios::end);
        in.read(reinterpret_cast<char*>(&file.header),EncodedBlockedGridVolumeFile::HeaderSize);
    }
    void ReadBlockInfoMap(){
        in.seekg(file.header.block_info_offset,std::ios::beg);
        std::vector<EncodedBlockedGridVolumeFile::BlockInfo> block_infos;
        block_infos.resize(file.header.block_info_size);
        in.read(reinterpret_cast<char*>(block_infos.data()),block_infos.size());
        for(auto& block_info:block_infos){
            file.mp[block_info.index] = block_info;
        }
    }
    void ReadRawBlock(const BlockIndex& blockIndex,Packets& packets){
        const auto& block_info = file.mp[blockIndex];
        in.seekg(block_info.offset);
        std::vector<uint8_t> bits;
        bits.resize(block_info.size);
        in.read(reinterpret_cast<char*>(bits.data()),bits.size());
        packets.resize(block_info.packet_count);
        size_t offset = 0;
        for(size_t i = 0; i < block_info.packet_count; i++){
            assert(offset < block_info.size);
            size_t packet_size = *reinterpret_cast<size_t*>(bits.data() + offset);
            offset += sizeof(packet_size);
            std::copy(bits.data() + offset,bits.data() + offset + packet_size,packets[i].data());
            offset += packet_size;
        }
        assert(offset == block_info.size);
    }
};

EncodedBlockedGridVolumeReader::EncodedBlockedGridVolumeReader(const std::string &filename) {
    _->in.open(filename,std::ios::binary);
    if(!_->in.is_open()){
        throw std::runtime_error("failed to open file: "+filename);
    }

    _->ReadHeader();
    if(!CheckValidation(_->file)){
        throw std::runtime_error("invalid context read from file");
    }
    _->ReadBlockInfoMap();
}

EncodedBlockedGridVolumeReader::~EncodedBlockedGridVolumeReader() {
    Close();
}

void EncodedBlockedGridVolumeReader::Close() {
    if(_->in.is_open()){
        _->in.close();
    }
}

const EncodedBlockedGridVolumeDesc &EncodedBlockedGridVolumeReader::GetEncodedBlockedGridVolumeDesc() const {
    return _->file.header.desc;
}

void EncodedBlockedGridVolumeReader::ReadBlockData(const BlockIndex &blockIndex, void *buf, DecodeWorker worker) {
    if(worker == nullptr){
        worker = [&](const Packets& packets,void* buf){
            bool e = DecodeBits(packets,buf,
                                GetVoxelBits(_->file.header.desc.type),
                                GetVoxelSampleCount(_->file.header.desc.format));
            return e;
        };
    }
    Packets packets;
    ReadRawBlockData(blockIndex,packets);
    bool e = worker(packets,buf);
    if(!e){
        std::cerr<<"read raw block failed: "<<blockIndex.x<<" "<<blockIndex.y<<" "<<blockIndex.z<<std::endl;
    }
}

void EncodedBlockedGridVolumeReader::ReadRawBlockData(const BlockIndex &blockIndex, Packet &packet) {
    std::cerr<<"this method is not implied at current!"<<std::endl;
}

void EncodedBlockedGridVolumeReader::ReadRawBlockData(const BlockIndex &blockIndex, Packets &packets) {
    if(_->file.mp.count(blockIndex) == 0){
        std::cerr<<"not find the block: "<<blockIndex.x<<" "<<blockIndex.y<<" "<<blockIndex.z<<std::endl;
        return;
    }
    _->ReadRawBlock(blockIndex,packets);
}

VOL_END