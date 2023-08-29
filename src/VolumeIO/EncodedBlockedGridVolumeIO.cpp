#include <VolumeUtils/Volume.hpp>
#include "../Common/Common.hpp"
#include "../Common/Utils.hpp"
#include <json.hpp>
#include <algorithm>
#include <fstream>
#include <source_location>

VOL_BEGIN

namespace{

#define ENCODED_BLOCKED_GRID_VOLUME_FILE_ID 0x7ffffebfLL
#define MAKE_VERSION(x,y,z) ((x << 32) | (y << 16) | z)
#define ENCODED_BLOCKED_GRID_VOLUME_FILE_VERSION MAKE_VERSION(1uLL,0uLL,0uLL)
#define INVALID_BLOCK_INDEX 0x7f7f7f7f
#define META_FILE_HEADER_SIZE 128ull
    // store according to space, use xierbote curve?
    class EncodedBlockedGridVolumeFile{
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
            fs.seekg(-static_cast<int64_t>(HeaderSize), std::ios::end);
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
            if(!fs.is_open()) return;
            header.block_info_count = mp.size();
            header.block_info_size = header.block_info_count * sizeof(BlockInfo);

            fs.seekp(0, std::ios::end);

            header.block_info_offset = fs.tellp();

            // write BlockInfos
            for(auto& b : mp){
                fs.write(reinterpret_cast<const char*>(&b.second),BlockInfoSize);
            }
            // write Header in the file end

            fs.write(reinterpret_cast<const char*>(&header), HeaderSize);

            fs.close();
        }
    public:
        EncodedBlockedGridVolumeFile() = default;

        ~EncodedBlockedGridVolumeFile(){
            SaveMetaFile();
            Close();
        }

        bool Open(const std::string& filename){
            io.open(filename, std::ios::in);
            if(!io.is_open()){
                return false;
            }

            nlohmann::json j;
            io >> j;
            if(j.count("desc") == 0){

                return false;
            }
            auto& encoded_block = j.at("desc");

            ReadDescFromJson(desc, encoded_block);

            auto ret = OpenMetaFile(desc.data_path);

            return ret;
        }

        bool Open(const std::string& filename,const EncodedBlockedGridVolumeDesc& volume_desc){
            io.open(filename, std::ios::out);
            if(!io.is_open()){
                std::cerr << "Saved EncodedBlockedGridVolumeDesc file open failed : " << filename << std::endl;
                return false;
            }
            if(!CheckValidation(volume_desc)){
                PrintVolumeDesc(volume_desc);
                return false;
            }
            this->desc = volume_desc;

            nlohmann::json j;
            using namespace detail;
            auto& encoded_block = j["desc"];
            encoded_block[volume_name] = desc.volume_name;
            encoded_block[voxel_type] = VoxelTypeToStr(desc.voxel_info.type);
            encoded_block[voxel_format] = VoxelFormatToStr(desc.voxel_info.format);
            encoded_block[extend] = {desc.extend.width, desc.extend.height, desc.extend.depth};
            encoded_block[space] = {desc.space.x, desc.space.y, desc.space.z};
            encoded_block[block_length] = desc.block_length;
            encoded_block[padding] = desc.padding;
            encoded_block[volume_codec] = VolumeCodecToStr(desc.codec);
            encoded_block[data_path] = desc.data_path;

            header.block_length = desc.block_length;
            header.padding = desc.padding;

            io << j;
            io.flush();

            fs.open(desc.data_path, std::ios::out | std::ios::binary);
            if(!fs.is_open()) return false;
            return true;
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
            if(!fs.is_open()) return;
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
        void Close(){
            if(fs.is_open())
                fs.close();
            if(io.is_open())
                io.close();
        }
    private:
        EncodedBlockedGridVolumeDesc desc;
        std::fstream io;

        std::unordered_map<BlockIndex, BlockInfo> mp;
        Header header;
        std::fstream fs;
    };
}

class EncodedBlockedGridVolumeReaderPrivate{
public:
    EncodedBlockedGridVolumeDesc desc;
    std::unique_ptr<CVolumeCodecInterface> video_codec;
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

    _->video_codec = CreateCPUVolumeVideoCodecByVoxel(_->desc.voxel_info);
    if(!_->video_codec){
        throw VolumeFileContextError("Failed to create volume video codec");
    }
}

EncodedBlockedGridVolumeReader::~EncodedBlockedGridVolumeReader() {

}

EncodedBlockedGridVolumeDesc EncodedBlockedGridVolumeReader::GetVolumeDesc() const noexcept {
    return _->desc;
}

void EncodedBlockedGridVolumeReader::ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, void *buf) {
    assert(srcX < dstX && srcY < dstY && srcZ < dstZ && buf);
#ifndef HIGH_PERFORMANCE
    size_t voxel_size = GetVoxelSize(_->desc.voxel_info);
    auto copy_func = GetCopyBitsFunc(voxel_size);
    ReadVolumeData(srcX, srcY, srcZ, dstX, dstY, dstZ,
                          [dst_ptr = reinterpret_cast<uint8_t*>(buf),
                           width = dstX - srcX, height = dstY - srcY, &copy_func]
                          (int dx, int dy, int dz, const void* src, size_t ele_size){
          size_t dst_offset = ((size_t)dz * width * height + (size_t)dy * width + dx) * ele_size;
          copy_func(reinterpret_cast<const uint8_t*>(src), dst_ptr + dst_offset);
    });
#else
    const int block_length = _->desc.block_length;
    const int padding = _->desc.padding;
    const int buffer_length = block_length + padding * 2;
    auto get_index = [block_length](int x){
        return x / block_length;
    };
    //[src, dst)
    //[beg, end)
    const int width = _->desc.extend.width;
    const int height = _->desc.extend.height;
    const int depth = _->desc.extend.depth;
    const int beg_block_x = get_index(std::max<int>(0, srcX + padding));
    const int end_block_x = get_index(std::min<int>(width, dstX - padding) + block_length - 1);
    const int beg_block_y = get_index(std::max<int>(0, srcY + padding));
    const int end_block_y = get_index(std::min<int>(height, dstY - padding) + block_length - 1);
    const int beg_block_z = get_index(std::max<int>(0, srcZ + padding));
    const int end_block_z = get_index(std::min<int>(depth, dstZ - padding) + block_length - 1 );

    size_t voxel_size = GetVoxelSize(_->desc.voxel_info);
    const auto src_ptr = _->block_data.data();
    auto fill_reader = [&, block_size = buffer_length](const BlockIndex& block_idx){
        assert(_->CheckValidation(block_idx));
        const int beg_x = std::max<int>(block_idx.x * block_length - padding, srcX);
        const int end_x = std::min<int>((block_idx.x + 1) * block_length + padding, dstX);
        const int beg_y = std::max<int>(block_idx.y * block_length - padding, srcY);
        const int end_y = std::min<int>((block_idx.y + 1) * block_length + padding, dstY);
        const int beg_z = std::max<int>(block_idx.z * block_length - padding, srcZ);
        const int end_z = std::min<int>((block_idx.z + 1) * block_length + padding, dstZ);
        const int block_size2 = block_size * block_size;
        size_t x_voxel_size = (end_x - beg_x) * voxel_size;
        for(int z = beg_z; z < end_z; z++){
            for(int y = beg_y; y < end_y; y++){
                size_t src_offset = ((size_t)block_size2 * (z - beg_z ) + block_size * (y - beg_y ) ) * voxel_size;
                size_t dst_offset =  ((size_t)(z - srcZ) * (dstX - srcX) * (dstY - srcY) + (size_t)(y - srcY) * (dstX - srcX) + beg_x - srcX) * voxel_size;
                std::memcpy(reinterpret_cast<uint8_t*>(buf) + dst_offset, src_ptr + src_offset, x_voxel_size);
            }
        }
    };
    VOL_WHEN_DEBUG(std::cout << "start read volume data" << std::endl)
    for(int block_z = beg_block_z; block_z < end_block_z; block_z++){
        for(int block_y = beg_block_y; block_y < end_block_y; block_y++){
            for(int block_x = beg_block_x; block_x < end_block_x; block_x++){
                const BlockIndex block_idx = {block_x, block_y, block_z};
                ReadBlockData(block_idx, _->block_data.data());
                fill_reader(block_idx);
                VOL_WHEN_DEBUG(std::cout << "read block : " << block_idx << std::endl)
            }
        }
    }
    VOL_WHEN_DEBUG(std::cout << "finish read volume data" << std::endl;)
#endif
}

void EncodedBlockedGridVolumeReader::ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadFunc reader) {
    assert(srcX < dstX && srcY < dstY && srcZ < dstZ && reader);

    const int block_length = _->desc.block_length;
    const int padding = _->desc.padding;
    const int buffer_length = block_length + padding * 2;
    auto get_index = [block_length](int x){
        if(x >= 0) return (x + block_length - 1) / block_length;
        return (x - block_length) / block_length;
    };
    //[src, dst)
    //[beg, end)
    const int width = _->desc.extend.width;
    const int height = _->desc.extend.height;
    const int depth = _->desc.extend.depth;
    const int beg_block_x = get_index(std::max<int>(0, srcX + padding));
    const int end_block_x = get_index(std::min<int>(width, dstX - padding) + block_length - 1);
    const int beg_block_y = get_index(std::max<int>(0, srcY + padding));
    const int end_block_y = get_index(std::min<int>(height, dstY - padding) + block_length - 1);
    const int beg_block_z = get_index(std::max<int>(0, srcZ + padding));
    const int end_block_z = get_index(std::min<int>(depth, dstZ - padding) + block_length - 1 );

    size_t voxel_size = GetVoxelSize(_->desc.voxel_info);
    const auto src_ptr = _->block_data.data();
    auto fill_reader = [&, block_size = buffer_length](const BlockIndex& block_idx){
        assert(_->CheckValidation(block_idx));
        const int beg_x = std::max<int>(block_idx.x * block_length - padding, srcX);
        const int end_x = std::min<int>((block_idx.x + 1) * block_length + padding, dstX);
        const int beg_y = std::max<int>(block_idx.y * block_length - padding, srcY);
        const int end_y = std::min<int>((block_idx.y + 1) * block_length + padding, dstY);
        const int beg_z = std::max<int>(block_idx.z * block_length - padding, srcZ);
        const int end_z = std::min<int>((block_idx.z + 1) * block_length + padding, dstZ);
        const int block_size2 = block_size * block_size;
        for(int z = beg_z; z < end_z; z++){
            for(int y = beg_y; y < end_y; y++){
                for(int x = beg_x; x < end_x; x++){
                    size_t src_offset = ((size_t)block_size2 * (z - beg_z) + block_size * (y - beg_y ) + (x - beg_x)) * voxel_size;
                    reader(x - srcX, y - srcY, z - srcZ, src_ptr + src_offset, voxel_size);
                }
            }
        }
    };

    for(int block_z = beg_block_z; block_z < end_block_z; block_z++){
        for(int block_y = beg_block_y; block_y < end_block_y; block_y++){
            for(int block_x = beg_block_x; block_x < end_block_x; block_x++){
                const BlockIndex block_idx = {block_x, block_y, block_z};
                ReadBlockData(block_idx, _->block_data.data());
                fill_reader(block_idx);
            }
        }
    }

}

void EncodedBlockedGridVolumeReader::ReadBlockData(const BlockIndex &blockIndex, void *buf) {
    assert(_->CheckValidation(blockIndex) && buf);


    Packets packets;
    ReadEncodedBlockData(blockIndex, packets);
    // invoke ReadBlockData next is ok but may loss efficient
    const uint32_t bl = _->desc.block_length + 2 * _->desc.padding;
    _->video_codec->Decode({bl, bl, bl}, packets, buf, _->block_bytes);
//    if(blockIndex == BlockIndex{1, 2, 1}){
//        std::ofstream out("H:/Volume/test_decoding#block1#2#1#_256_256_256_uint8.raw", std::ios::binary);
//        out.write(reinterpret_cast<const char*>(_->block_data.data()), _->block_bytes);
//        out.close();
//        std::cout << "write block 1,2,1 to file" << std::endl;
//    }
}

void EncodedBlockedGridVolumeReader::ReadBlockData(const BlockIndex &blockIndex, VolumeReadFunc reader) {
    assert(_->CheckValidation(blockIndex) && reader);

    ReadBlockData(blockIndex, _->block_data.data());

    const int block_length = _->desc.block_length;
    const int padding = _->desc.padding;
    const int buffer_length = block_length + 2 * padding;
    const auto src_ptr = _->block_data.data();
    size_t voxel_size = GetVoxelSize(_->desc.voxel_info);
    for(int z = 0; z < buffer_length; z++){
        for(int y = 0; y < buffer_length; y++){
            for(int x = 0; x < buffer_length; x++){
                size_t src_offset = ((size_t)buffer_length * buffer_length * z + buffer_length * y + x) * voxel_size;
                reader(x, y, z, src_ptr + src_offset, voxel_size);
            }
        }
    }
}

size_t EncodedBlockedGridVolumeReader::ReadEncodedBlockData(const BlockIndex &blockIndex, Packets &packets) {
    assert(_->CheckValidation(blockIndex));

    auto size = _->file.GetBlockSize(blockIndex);
    uint8_t* ptr;
    std::vector<uint8_t> tmp;
    if(size <= _->block_bytes) ptr = _->block_data.data();
    else {
        tmp.resize(size, 0);
        ptr = tmp.data();
    }
    auto ret = _->file.ReadBlock(blockIndex, ptr, size);
    size_t offset = 0;
    while(offset < size){
        size_t packet_size = *reinterpret_cast<size_t*>(ptr + offset);
        auto& packet_buffer = packets.emplace_back();
        packet_buffer.resize(packet_size, 0);
        offset += 8;
        std::memcpy(packet_buffer.data(), ptr + offset , packet_size);
        offset += packet_size;
    }
    assert(ret == offset);
    return ret;
}

size_t EncodedBlockedGridVolumeReader::ReadEncodedBlockData(const BlockIndex &blockIndex, void *buf, size_t size) {
    assert(_->CheckValidation(blockIndex));
    return _->file.ReadBlock(blockIndex, buf, size);
}

class EncodedBlockedGridVolumeWriterPrivate{
public:
    EncodedBlockedGridVolumeDesc desc;
    std::unique_ptr<CVolumeCodecInterface> video_codec;
    EncodedBlockedGridVolumeFile file;

    size_t block_bytes;
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

EncodedBlockedGridVolumeWriter::EncodedBlockedGridVolumeWriter(const std::string &filename, const EncodedBlockedGridVolumeDesc &desc) {
    _ = std::make_unique<EncodedBlockedGridVolumeWriterPrivate>();
    if(!CheckValidation(desc)){
        throw VolumeFileContextError("Invalid EncodedBlockedGridVolumeDesc");
    }
    if(!_->file.Open(filename, desc)){
        throw VolumeFileOpenError("EncodedBlockedGridVolumeFile open failed : " + filename);
    }

    _->desc = _->file.GetVolumeDesc();
    const size_t buffer_length = _->desc.block_length + _->desc.padding * 2;
    _->block_bytes = buffer_length * buffer_length * buffer_length * GetVoxelSize(_->desc.voxel_info);
    _->block_data.resize(_->block_bytes, 0);

    _->video_codec = CreateCPUVolumeVideoCodecByVoxel(_->desc.voxel_info);
    if(!_->video_codec){
        throw VolumeFileContextError("Failed to create volume video codec");
    }
}

EncodedBlockedGridVolumeWriter::~EncodedBlockedGridVolumeWriter() {

}

EncodedBlockedGridVolumeDesc EncodedBlockedGridVolumeWriter::GetVolumeDesc() const noexcept {
    return _->desc;
}

void EncodedBlockedGridVolumeWriter::WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeWriteFunc writer) {
    assert(srcX < dstX && srcY < dstY && srcZ < dstZ && writer);

    const int block_length = _->desc.block_length;
    const int padding = _->desc.padding;
    const int buffer_length = block_length + padding * 2;
    auto get_index = [block_length](int x){
        if(x >= 0) return (x + block_length - 1) / block_length;
        return (x - block_length) / block_length;
    };
    //[src, dst)
    //[beg, end)
    const int width = _->desc.extend.width;
    const int height = _->desc.extend.height;
    const int depth = _->desc.extend.depth;
    const int beg_block_x = get_index(std::max<int>(0, srcX + padding));
    const int end_block_x = get_index(std::min<int>(width, dstX - padding) + block_length - 1);
    const int beg_block_y = get_index(std::max<int>(0, srcY + padding));
    const int end_block_y = get_index(std::min<int>(height, dstY - padding) + block_length - 1);
    const int beg_block_z = get_index(std::max<int>(0, srcZ + padding));
    const int end_block_z = get_index(std::min<int>(depth, dstZ - padding) + block_length - 1 );

    size_t voxel_size = GetVoxelSize(_->desc.voxel_info);
    const auto dst_ptr = _->block_data.data();
    auto fill_writer = [&, block_size = buffer_length](const BlockIndex& block_idx){
        assert(_->CheckValidation(block_idx));
        const int beg_x = std::max<int>(block_idx.x * block_length - padding, srcX);
        const int end_x = std::min<int>((block_idx.x + 1) * block_length + padding, dstX);
        const int beg_y = std::max<int>(block_idx.y * block_length - padding, srcY);
        const int end_y = std::min<int>((block_idx.y + 1) * block_length + padding, dstY);
        const int beg_z = std::max<int>(block_idx.z * block_length - padding, srcZ);
        const int end_z = std::min<int>((block_idx.z + 1) * block_length + padding, dstZ);
        const int block_size2 = block_size * block_size;
        for(int z = beg_z; z < end_z; z++){
            for(int y = beg_y; y < end_y; y++){
                for(int x = beg_x; x < end_x; x++){
                    size_t dst_offset = ((size_t)block_size2 * (z - beg_z) + block_size * (y - beg_y) + (x - beg_x)) * voxel_size;
                    writer(x - srcX, y - srcY, z - srcZ, dst_ptr + dst_offset, voxel_size);
                }
            }
        }
    };
    for(int block_z = beg_block_z; block_z < end_block_z; block_z++){
        for(int block_y = beg_block_y; block_y < end_block_y; block_y++){
            for(int block_x = beg_block_x; block_x < end_block_x; block_x++){
                const BlockIndex block_idx = {block_x, block_y, block_z};
                fill_writer(block_idx);
                WriteBlockData(block_idx, _->block_data.data());
            }
        }
    }
}

void EncodedBlockedGridVolumeWriter::WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const void *buf) {
    assert(srcX < dstX && srcY < dstY && srcZ < dstZ && buf);
#ifndef HIGH_PERFORMANCE
    size_t voxel_size = GetVoxelSize(_->desc.voxel_info);
    auto copy_func = GetCopyBitsFunc(voxel_size);
    return WriteVolumeData(srcX, srcY, srcZ, dstX, dstY, dstZ,
                           [src_ptr = reinterpret_cast<const uint8_t*>(buf),
                            width = dstX - srcX, height = dstY - srcY, &copy_func]
                            (int dx, int dy, int dz, void* dst, size_t ele_size){
            size_t src_offset = ((size_t)dz * width * height + (size_t)dy * width + dx) * ele_size;
            copy_func(src_ptr + src_offset, reinterpret_cast<uint8_t*>(dst));
    });
#else
    const int block_length = _->desc.block_length;
    const int padding = _->desc.padding;
    const int buffer_length = block_length + padding * 2;
    auto get_index = [block_length](int x){
        if(x >= 0) return (x + block_length - 1) / block_length;
        return (x - block_length) / block_length;
    };
    //[src, dst)
    //[beg, end)
    const int width = _->desc.extend.width;
    const int height = _->desc.extend.height;
    const int depth = _->desc.extend.depth;
    const int beg_block_x = get_index(std::max<int>(0, srcX + padding));
    const int end_block_x = get_index(std::min<int>(width, dstX - padding) + block_length - 1);
    const int beg_block_y = get_index(std::max<int>(0, srcY + padding));
    const int end_block_y = get_index(std::min<int>(height, dstY - padding) + block_length - 1);
    const int beg_block_z = get_index(std::max<int>(0, srcZ + padding));
    const int end_block_z = get_index(std::min<int>(depth, dstZ - padding) + block_length - 1 );

    size_t voxel_size = GetVoxelSize(_->desc.voxel_info);
    const auto dst_ptr = _->block_data.data();
    auto fill_writer = [&, block_size = buffer_length](const BlockIndex& block_idx){
        assert(_->CheckValidation(block_idx));
        const int beg_x = std::max<int>(block_idx.x * block_length - padding, srcX);
        const int end_x = std::min<int>((block_idx.x + 1) * block_length + padding, dstX);
        const int beg_y = std::max<int>(block_idx.y * block_length - padding, srcY);
        const int end_y = std::min<int>((block_idx.y + 1) * block_length + padding, dstY);
        const int beg_z = std::max<int>(block_idx.z * block_length - padding, srcZ);
        const int end_z = std::min<int>((block_idx.z + 1) * block_length + padding, dstZ);
        const int block_size2 = block_size * block_size;
        size_t x_voxel_size = (end_x - beg_x) * voxel_size;
        for(int z = beg_z; z < end_z; z++){
            for(int y = beg_y; y < end_y; y++){
                size_t dst_offset = ((size_t)block_size2 * (z - beg_z) + block_size * (y - beg_y)) * voxel_size;
                size_t src_offset = ((size_t)(z - srcZ) * (dstY - srcY) * (dstX - srcX) + (y - srcY) * (dstX - srcX) + beg_z - srcX) * voxel_size;
                std::memcpy(dst_ptr + dst_offset, reinterpret_cast<const uint8_t*>(buf) + src_offset, x_voxel_size);
            }
        }
    };
    for(int block_z = beg_block_z; block_z < end_block_z; block_z++){
        for(int block_y = beg_block_y; block_y < end_block_y; block_y++){
            for(int block_x = beg_block_x; block_x < end_block_x; block_x++){
                const BlockIndex block_idx = {block_x, block_y, block_z};
                fill_writer(block_idx);
                WriteBlockData(block_idx, _->block_data.data());
            }
        }
    }
#endif
}

void EncodedBlockedGridVolumeWriter::WriteBlockData(const BlockIndex &blockIndex, VolumeWriteFunc writer) {
#ifdef VOL_DEBUG
    auto startTime = std::chrono::system_clock::now();
#endif // VOL_DEBUG

    assert(_->CheckValidation(blockIndex) && writer);

    size_t voxel_size = GetVoxelSize(_->desc.voxel_info);
    const int block_length = _->desc.block_length;
    const int padding = _->desc.padding;
    const int buffer_length = block_length + 2 * padding;
    auto dst_ptr = _->block_data.data();

    for(int z = 0; z < buffer_length; z++){
        parallel_forrange(0, buffer_length, [&](int thread_idx, int y){
            for(int x = 0; x < buffer_length; x++){
                size_t dst_offset = ((size_t)buffer_length * buffer_length * z + buffer_length * y + x) * voxel_size;
                writer(x, y, z, dst_ptr + dst_offset, voxel_size);
            }
        });

//        for(int y = 0; y < buffer_length; y++){
//            for(int x = 0; x < buffer_length; x++){
//                size_t dst_offset = ((size_t)buffer_length * buffer_length * z + buffer_length * y + x) * voxel_size;
//                writer(x, y, z, dst_ptr + dst_offset, voxel_size);
//            }
//        }
    }

    
#ifdef VOL_DEBUG
    auto endTime = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    std::cout << std::format("{} takes {}.\n", std::source_location::current().function_name(), duration);
#endif // VOL_DUBUG


//    if(blockIndex == BlockIndex{1, 2, 1}){
//        std::ofstream out("H:/Volume/test_block1#2#1#_256_256_256_uint8.raw", std::ios::binary);
//        out.write(reinterpret_cast<const char*>(_->block_data.data()), _->block_bytes);
//        out.close();
//        std::cout << "write block 1,2,1 to file" << std::endl;
//    }
    WriteBlockData(blockIndex, _->block_data.data());
}

void EncodedBlockedGridVolumeWriter::WriteBlockData(const BlockIndex &blockIndex, const void *buf) {
#ifdef VOL_DEBUG
    auto startTime = std::chrono::system_clock::now();
#endif // VOL_DEBUG

    assert(_->CheckValidation(blockIndex) && buf);    

    Packets packets;
    const uint32_t bl = _->desc.block_length + 2 * _->desc.padding;
    _->video_codec->Encode({bl, bl, bl}, buf, _->block_bytes, packets);
//    VOL_WHEN_DEBUG({
//        auto p = reinterpret_cast<const uint8_t*>(buf);
//                       std::vector<uint8_t> table(256, 0);
//                       for(int i = 0; i < _->block_bytes; i++){
//                           table[p[i]]++;
//                       }
//                       for(int i = 0; i < 256; i++)
//                           std::cout << "(" << i << " : " << (int)(table[i]) << ")" << std::endl;
//                   })

#ifdef VOL_DEBUG
    auto endTime = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    std::cout << std::format("{} takes {}.\n", std::source_location::current().function_name(), duration);
#endif // VOL_DUBUG

    WriteEncodedBlockData(blockIndex, packets);
}

void EncodedBlockedGridVolumeWriter::WriteEncodedBlockData(const BlockIndex &blockIndex, const Packets &packets) {
#ifdef VOL_DEBUG
    auto startTime = std::chrono::system_clock::now();
#endif // VOL_DEBUG

    assert(_->CheckValidation(blockIndex));

    size_t buf_size = packets.size() * sizeof(size_t);
    for(auto& packet : packets) buf_size += packet.size();
    uint8_t* buf = nullptr;
    std::vector<uint8_t> tmp;
    if(buf_size > _->block_bytes){
        tmp.resize(buf_size);
        buf = tmp.data();
    }
    else{
        buf = _->block_data.data();
    }
    size_t offset = 0;
    for(auto& packet : packets){
        auto p = reinterpret_cast<size_t*>(buf + offset);
        *p = packet.size();
        offset += 8;
        std::memcpy(buf + offset, packet.data(), packet.size());
        offset += packet.size();
    }
    assert(offset == buf_size);
    
#ifdef VOL_DEBUG
    auto endTime = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    std::cout << std::format("{} takes {}.\n", std::source_location::current().function_name(), duration);
#endif // VOL_DUBUG

    WriteEncodedBlockData(blockIndex, buf, buf_size);
}

void EncodedBlockedGridVolumeWriter::WriteEncodedBlockData(const BlockIndex &blockIndex, const void *buf, size_t size) {
#ifdef VOL_DEBUG
    auto startTime = std::chrono::system_clock::now();
#endif // VOL_DEBUG

    assert(_->CheckValidation(blockIndex) && buf && size);

    _->file.WriteBlock(blockIndex, buf, size);

#ifdef VOL_DEBUG
    auto endTime = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    std::cout << std::format("{} takes {}.\n", std::source_location::current().function_name(), duration);
#endif // VOL_DUBUG

    VOL_WHEN_DEBUG(std::cout << "write encode block size : " << size << std::endl)
}




VOL_END