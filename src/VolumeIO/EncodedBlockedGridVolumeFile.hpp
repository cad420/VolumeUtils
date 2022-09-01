#pragma once

#include "VolumeUtils/Volume.hpp"
#define ENCODED_BLOCKED_GRID_VOLUME_FILE_ID 0x7ffffebfLL
#define MAKE_VERSION(x,y,z) ((x << 32) | (y << 16) | z)
#define ENCODED_BLOCKED_GRID_VOLUME_FILE_VERSION MAKE_VERSION(1LL,0LL,0LL)
#define INVALID_BLOCK_INDEX 0x7fffffff
//packet0:packet_size#packet_data#packet_size#packet_data...#packet_size#packet_data
//packet1:packet_size#packet_data#packet_size#packet_data...#packet_size#packet_data
//...
VOL_BEGIN
struct EncodedBlockedGridVolumeFile{
    struct BlockInfo{
        BlockIndex index = {INVALID_BLOCK_INDEX,INVALID_BLOCK_INDEX,INVALID_BLOCK_INDEX};
        size_t offset = 0;//offset to file beg
        size_t size = 0;//total write file size for this block data, this is larger than encode size
        size_t packet_count = 0;//option for video encoded
        char preserve[24] = {0};
    };//64bytes
    static constexpr size_t BlockInfoSize = 64;
    static_assert(sizeof(BlockInfo)==BlockInfoSize,"");

    std::unordered_map<BlockIndex,BlockInfo> mp;

    struct Header{
        EncodedBlockedGridVolumeDesc desc;
        size_t file_id = ENCODED_BLOCKED_GRID_VOLUME_FILE_ID;
        size_t file_version = ENCODED_BLOCKED_GRID_VOLUME_FILE_VERSION;
        size_t block_info_offset = 0;
        uint32_t block_info_size = 0;//equal to block_info_count * BlockInfoSize
        uint32_t block_info_count = 0;
        char preserve[64] = {0};
    };
    static constexpr size_t HeaderSize = 160;
    static_assert(sizeof(Header) == HeaderSize,"");
    Header header;
};
inline bool CheckValidation(const EncodedBlockedGridVolumeFile& file){
    if(file.header.file_id != ENCODED_BLOCKED_GRID_VOLUME_FILE_ID
    || file.header.file_version != ENCODED_BLOCKED_GRID_VOLUME_FILE_VERSION){
        return false;
    }
    if(!CheckValidation(file.header.desc)){
        return false;
    }

    return true;
}


VOL_END