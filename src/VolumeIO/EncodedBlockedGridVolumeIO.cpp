#include <VolumeUtils/Volume.hpp>
#include "../Common/Common.hpp"
#include "../Common/Utils.hpp"
#include <json.hpp>
#include <algorithm>
#include <fstream>

VOL_BEGIN

namespace{

#define ENCODED_BLOCKED_GRID_VOLUME_FILE_ID 0x7ffffebfLL
#define MAKE_VERSION(x,y,z) ((x << 32) | (y << 16) | z)
#define ENCODED_BLOCKED_GRID_VOLUME_FILE_VERSION MAKE_VERSION(1uLL,0uLL,0uLL)
#define INVALID_BLOCK_INDEX 0x7f7f7f7f
#define META_FILE_HEADER_SIZE 128ull
    // store according to space, use xierbote curve?
    class EncodedBlockedGridVolumeFile{
        const char* encoded_blocked = "encoded_blocked";
        const char* volume_name = "volume_name";
        const char* voxel_type = "voxel_type";
        const char* voxel_format = "voxel_format";
        const char* extend = "extend";
        const char* space = "space";
        const char* block_length = "block_length";
        const char* padding = "padding";
        const char* volume_codec = "volume_codec";
        const char* data_path = "data_path";
        struct Header{
            size_t file_id;
            size_t file_version;
            uint32_t block_length;
            uint32_t padding;
            size_t block_info_offset;
            uint32_t block_info_count;
            uint32_t block_info_size; // equal to block_info_count * BlockInfoSize
            char preserve[88];
        };
        static constexpr size_t HeaderSize = META_FILE_HEADER_SIZE;
        static_assert(sizeof(Header) == HeaderSize, "");
        struct BlockInfo{
            BlockIndex index{INVALID_BLOCK_INDEX,INVALID_BLOCK_INDEX,INVALID_BLOCK_INDEX};
            size_t offset = 0; // offset to file beg
            size_t size = 0; // total write file size for this block data, this is larger than encode size
            size_t packet_count = 0; // option for video codec
            char preserve[24];
        };
        static constexpr size_t BlockInfoSize = 64;
        static_assert(sizeof(BlockInfo) == BlockInfoSize, "");

        bool OpenMetaFile(const std::string& filename){
            fs.open(filename, std::ios::in | std::ios::beg | std::ios::binary);
            if(!fs.is_open()){
                return false;
            }
            fs.read(reinterpret_cast<char*>(&header), HeaderSize);
            fs.seekg(header.block_info_offset, std::ios::beg);
            std::vector<BlockInfo> block_infos(header.block_info_count);
            fs.read(reinterpret_cast<char*>(block_infos.data()), header.block_info_size);
            for(auto& b : block_infos){
                mp[b.index] = b;
            }
            return true;
        }
        void SaveMetaFile(){

        }
    public:
        bool Open(const std::string& filename){
            io.open(filename, std::ios::in);
            if(!io.is_open()){
                return false;
            }

            nlohmann::json j;
            io >> j;
            if(j.count(encoded_blocked) == 0){

                return false;
            }
            auto encoded_block = j[encoded_blocked];

            desc.volume_name = encoded_block.count(volume_name) == 0 ? "none" : j.at(encoded_blocked);
            desc.voxel_info.type = StrToVoxelType(encoded_block.count(voxel_type) == 0 ? "unknown" : encoded_block.at(voxel_type));
            desc.voxel_info.format = StrToVoxelFormat(encoded_block.count(voxel_format) == 0 ? "none" : encoded_block.at(voxel_format));
            if(encoded_block.count(extend) != 0){
                std::array<int, 3> shape = encoded_block.at(extend);
                desc.extend = {(uint32_t)shape[0], (uint32_t)shape[1], (uint32_t)shape[2]};
            }
            if(encoded_block.count(space) != 0){
                std::array<float, 3> sp = encoded_block.at(space);
                desc.space = {sp[0], sp[1], sp[2]};
            }
            desc.block_length = encoded_block.count(block_length) == 0 ? 0 : (int)j[block_length];
            desc.padding = encoded_block.count(padding) == 0 ? 0 : (int)j[padding];
            desc.codec = StrToGridVolumeDataCodec(encoded_block.count(volume_codec) == 0 ? "none" : j.at(volume_codec));

            meta_data_path = encoded_block.count(data_path) == 0 ? "none" : j.at(data_path);

            auto ret = OpenMetaFile(meta_data_path);

            return ret;
        }

        bool Open(const std::string& filename,const EncodedBlockedGridVolumeDesc& volume_desc){

        }

        const EncodedBlockedGridVolumeDesc& GetVolumeDesc() const{
            return desc;
        }

        size_t ReadBlock(const BlockIndex& blockIndex, void* buf, size_t buf_size){
            if(mp.count(blockIndex) == 0) return 0;
            auto offset = mp.at(blockIndex).offset;
            auto block_size = mp.at(blockIndex).size;
            auto read_size = std::min(buf_size, block_size);
            fs.seekg(offset, std::ios::beg);
            fs.read(reinterpret_cast<char*>(buf), read_size);
            return read_size;
        }

        size_t GetBlockSize(const BlockIndex& blockIndex) const{
            if(mp.count(blockIndex) == 0) return 0;
            return mp.at(blockIndex).size;
        }

        void WriteBlock(const BlockIndex& blockIndex, const void* buf, size_t size, size_t packet_count = 0){
            if(mp.count(blockIndex) != 0) return;
            fs.seekp(0, std::ios::end);
            auto offset = fs.tellp();
            auto& block = mp[blockIndex];
            block.index = blockIndex;
            block.offset = offset;
            block.size = size;
            block.packet_count = packet_count;
            fs.write(reinterpret_cast<const char*>(buf), size);
        }
    private:
        EncodedBlockedGridVolumeDesc desc;
        std::fstream io;
        std::string meta_data_path;

        std::unordered_map<BlockIndex, BlockInfo> mp;
        Header header;
        std::fstream fs;
    };
}

std::unique_ptr<CVolumeVideoCodecInterface> CreateCPUVideoCodec(const VoxelInfo& voxel){
    auto [type, format] = voxel;
    if(type == VoxelType::uint8){
        if(format == VoxelFormat::R){
            return std::make_unique<VolumeVideoCodec<VoxelRU8,CodecDevice::CPU>>();
        }
    }
    else if(type == VoxelType::uint16){
        if(format == VoxelFormat::R){
            return std::make_unique<VolumeVideoCodec<VoxelRU16,CodecDevice::CPU>>();
        }
    }
    return nullptr;
}

class EncodedBlockedGridVolumeReaderPrivate{
public:
    EncodedBlockedGridVolumeDesc desc;
    std::unique_ptr<CVolumeVideoCodecInterface> video_codec;
    EncodedBlockedGridVolumeFile file;

    size_t block_bytes;
    // used for cache read buffer by ReadVolumeData, ReadBlockData will not use it.
    std::vector<uint8_t> block_data;

    bool CheckValidation(const BlockIndex& blockIndex) const{
        const auto block_length = desc.block_length;
        const auto block_x = (desc.extend.width + block_length - 1) / block_length;
        const auto block_y = (desc.extend.height + block_length - 1) / block_length;
        const auto block_z = (desc.extend.depth + block_length - 1) / block_length;
        return blockIndex.x >= 0 && blockIndex.x < block_x
            && blockIndex.y >= 0 && blockIndex.y < block_y
            && blockIndex.z >= 0 && blockIndex.z < block_z;
    }
};

EncodedBlockedGridVolumeReader::EncodedBlockedGridVolumeReader(const std::string &filename) {
    _ = std::make_unique<EncodedBlockedGridVolumeReaderPrivate>();

    if(!_->file.Open(filename)){
        throw VolumeFileOpenError("EncodedBlockedGridVolumeFile open failed : " + filename);
    }

    _->desc = _->file.GetVolumeDesc();
    const size_t buffer_length = _->desc.block_length + _->desc.padding * 2;
    _->block_bytes = buffer_length * buffer_length * buffer_length * GetVoxelSize(_->desc.voxel_info);
    _->block_data.resize(_->block_bytes, 0);

}

EncodedBlockedGridVolumeReader::~EncodedBlockedGridVolumeReader() {

}

const EncodedBlockedGridVolumeDesc &EncodedBlockedGridVolumeReader::GetVolumeDesc() const noexcept {
    return _->desc;
}

size_t EncodedBlockedGridVolumeReader::ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, void *buf, size_t size) noexcept {
    size_t voxel_size = GetVoxelSize(_->desc.voxel_info);
    auto copy_func = GetCopyBitsFunc(voxel_size);
    return ReadVolumeData(srcX, srcY, srcZ, dstX, dstY, dstZ,
                          [dst_ptr = reinterpret_cast<uint8_t*>(buf), buf_size = size,
                           width = dstX - srcX, height = dstY - srcY, &copy_func]
                          (int dx, int dy, int dz, const void* src, size_t ele_size)->size_t{
          size_t dst_offset = ((size_t)dz * width * height + (size_t)dy * width + dx) * ele_size;
          if(dst_offset >= buf_size) return 0;
          copy_func(reinterpret_cast<const uint8_t*>(src), dst_ptr + dst_offset);
          return ele_size;
    });
}

size_t EncodedBlockedGridVolumeReader::ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadFunc reader) noexcept {
    if(srcX >= dstX || srcY >= dstY || srcZ >= dstZ) return 0;
    if(!reader) return 0;

    const int block_length = _->desc.block_length;
    const int padding = _->desc.padding;
    const int buffer_length = block_length + padding * 2;
    auto get_index = [block_length](int x){
        if(x >= 0) return (x + block_length - 1) / block_length;
        return (x - block_length) / block_length;
    };

    const int beg_block_x = get_index(srcX);
    const int end_block_x = get_index(dstX) + 1;
    const int beg_block_y = get_index(srcY);
    const int end_block_y = get_index(dstY) + 1;
    const int beg_block_z = get_index(srcZ);
    const int end_block_z = get_index(dstZ) + 1;

    size_t filled_size = 0;
    size_t voxel_size = GetVoxelSize(_->desc.voxel_info);
    const auto src_ptr = _->block_data.data();
    auto fill_reader = [&](const BlockIndex& block_idx){
        assert(_->CheckValidation(block_idx));
        const int beg_x = block_idx.x * block_length - padding;
        const int end_x = beg_x + padding + block_length + padding;
        const int beg_y = block_idx.y * block_length - padding;
        const int end_y = beg_y + padding + block_length + padding;
        const int beg_z = block_idx.z * block_length - padding;
        const int end_z = beg_z + padding + block_length + padding;
        size_t read_size = 0;
        for(int z = beg_z; z < end_z; z++){
            for(int y = beg_y; y < end_y; y++){
                for(int x = beg_x; x < end_x; x++){
                    size_t src_offset = ((size_t)buffer_length * buffer_length * z + buffer_length * y + x) * voxel_size;
                    read_size += reader(x - srcX, y - srcY, z - srcZ, src_ptr + src_offset, voxel_size);
                }
            }
        }
        return read_size;
    };

    for(int block_z = beg_block_z; block_z < end_block_z; block_z++){
        for(int block_y = beg_block_y; block_y < end_block_y; block_y++){
            for(int block_x = beg_block_x; block_x < end_block_x; block_x++){
                const BlockIndex block_idx = {block_x, block_y, block_z};
                auto read = ReadBlockData(block_idx, _->block_data.data(), _->block_bytes);
                if(!read) continue;
                auto filled = fill_reader(block_idx);
                filled_size += filled;
            }
        }
    }
    return filled_size;
}

size_t EncodedBlockedGridVolumeReader::ReadBlockData(const BlockIndex &blockIndex, void *buf, size_t size) noexcept {
    // check blockIndex
    if(!_->CheckValidation(blockIndex)) return 0;
    if(!buf || !size) return 0;
    Packets packets;
    ReadEncodedBlockData(blockIndex, packets);
    // invoke ReadBlockData next is ok but may loss efficient
    return _->video_codec->Decode(packets, buf, size);
}

size_t EncodedBlockedGridVolumeReader::ReadBlockData(const BlockIndex &blockIndex, VolumeReadFunc reader) noexcept {
    if(!_->CheckValidation(blockIndex)) return 0;
    if(!reader) return 0;
    auto ret = ReadBlockData(blockIndex, _->block_data.data(), _->block_bytes);
    assert(ret == _->block_bytes);
    const int block_length = _->desc.block_length;
    const int padding = _->desc.padding;
    const int buffer_length = block_length + 2 * padding;
    const auto src_ptr = _->block_data.data();
    size_t voxel_size = GetVoxelSize(_->desc.voxel_info);
    size_t read_size = 0;
    for(int z = 0; z < buffer_length; z++){
        for(int y = 0; y < buffer_length; y++){
            for(int x = 0; x < buffer_length; x++){
                size_t src_offset = ((size_t)buffer_length * buffer_length * z + buffer_length * y + x) * voxel_size;
                auto filled = reader(x, y, z, src_ptr + src_offset, voxel_size);
                read_size += filled;
            }
        }
    }
    return read_size;
}

size_t EncodedBlockedGridVolumeReader::ReadEncodedBlockData(const BlockIndex &blockIndex, Packets &packets) noexcept {
    if(!_->CheckValidation(blockIndex)) return 0;
    auto size = _->file.GetBlockSize(blockIndex);
    uint8_t* ptr;
    std::vector<uint8_t> tmp;
    if(size <= _->block_bytes) ptr = _->block_data.data();
    else {
        tmp.resize(size, 0);
        ptr = tmp.data();
    }
    auto ret = _->file.ReadBlock(blockIndex, ptr, size);
    assert(ret == size);
    size_t offset = 0;
    size_t read_size = 0;
    while(offset < size){
        size_t packet_size = reinterpret_cast<size_t*>(ptr)[offset];
        read_size += packet_size;
        auto& packet_buffer = packets.emplace_back();
        packet_buffer.resize(packet_size, 0);
        offset += 8;
        std::memcpy(packet_buffer.data(), ptr + offset , packet_size);
        offset += packet_size;
    }
    return read_size;
}

size_t EncodedBlockedGridVolumeReader::ReadEncodedBlockData(const BlockIndex &blockIndex, void *buf, size_t size) noexcept {
    if(!_->CheckValidation(blockIndex)) return 0;
    return _->file.ReadBlock(blockIndex, buf, size);
}

class EncodedBlockedGridVolumeWriterPrivate{
public:

};

EncodedBlockedGridVolumeWriter::EncodedBlockedGridVolumeWriter(const std::string &filename) {

}

EncodedBlockedGridVolumeWriter::~EncodedBlockedGridVolumeWriter() {

}

void EncodedBlockedGridVolumeWriter::SetVolumeDesc(const EncodedBlockedGridVolumeDesc &) noexcept {

}

const EncodedBlockedGridVolumeDesc &EncodedBlockedGridVolumeWriter::GetVolumeDesc() const noexcept {
    return {};
}

void EncodedBlockedGridVolumeWriter::WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeWriteFunc writer) noexcept {

}

void EncodedBlockedGridVolumeWriter::WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const void *buf, size_t size) noexcept {

}

void EncodedBlockedGridVolumeWriter::WriteBlockData(const BlockIndex &blockIndex, VolumeWriteFunc writer) noexcept {

}

void EncodedBlockedGridVolumeWriter::WriteBlockData(const BlockIndex &blockIndex, const void *buf, size_t size) noexcept {

}

void EncodedBlockedGridVolumeWriter::WriteEncodedBlockData(const BlockIndex &blockIndex, const Packets &packets) noexcept {

}

void EncodedBlockedGridVolumeWriter::WriteEncodedBlockData(const BlockIndex &blockIndex, const void *buf, size_t size) noexcept {

}



VOL_END