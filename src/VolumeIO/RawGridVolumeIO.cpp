#include <VolumeUtils/Volume.hpp>

#include "../Common/Utils.hpp"
#include "../Common/Common.hpp"

#include <fstream>
#include <iostream>
#include <json.hpp>

VOL_BEGIN

class RawGridVolumeFile {

    std::pair<RawGridVolumeDesc, bool> ExtractDescFromNameStr(const std::string &filename) {
        return {{}, false};
    }

    std::pair<RawGridVolumeDesc, bool> ExtractDescFromFile(const std::string &filename) {
        // read from json file
        RawGridVolumeDesc desc;

        std::ifstream in(filename);
        if(!in.is_open()) return {desc, false};
        nlohmann::json j;
        in >> j;

        auto& raw_ = j.at("desc");

        ReadDescFromJson(desc, raw_);

        return {desc, true};
    }

public:

    RawGridVolumeFile() = default;

    bool Open(const std::string &filename){
        auto [desc0, ok0] = ExtractDescFromNameStr(filename);
        if (ok0) {
            this->raw_desc = desc0;
        } else {
            auto [desc1, ok1] = ExtractDescFromFile(filename);
            if (ok1) {
                this->raw_desc = desc1;
            } else {
                throw VolumeFileOpenError("RawGridVolumeFile Open Error: " + filename);
            }
        }
    }

    bool Save(const std::string &filename, const RawGridVolumeDesc& desc){
        assert(CheckValidation(desc));
        if(out.is_open()) out.close();

        out.open(filename);
        if(!out.is_open()){
            std::cerr << "Open file failed : " << filename << std::endl;
            return false;
        }

        raw_desc = desc;

        nlohmann::json jj;

        auto& j = jj["desc"];
        using namespace detail;
        j[volume_name]  = desc.volume_name;
        j[voxel_type]   = VoxelTypeToStr(desc.voxel_info.type);
        j[voxel_format] = VoxelFormatToStr(desc.voxel_info.format);
        j[extend]       = {desc.extend.width, desc.extend.height, desc.extend.depth};
        j[space]        = {desc.space.x, desc.space.y, desc.space.z};
        j[data_path]    = desc.data_path;

        out << jj;
        out.flush();
        return true;
    }

    [[nodiscard]]
    const RawGridVolumeDesc &GetVolumeDesc() const {
        return raw_desc;
    }

    [[nodiscard]]
    const std::string &GetDataPath() const {
        return raw_desc.data_path;
    }

private:
    RawGridVolumeDesc raw_desc;
    std::ofstream out;
};

class RawGridVolumeReaderPrivate {
public:
#ifndef USE_MAPPING_FILE
    // file not so large
    RawGridVolumeDesc desc;
    RawGridVolumeFile file;
    std::ifstream in;
#else

#endif
};

RawGridVolumeReader::RawGridVolumeReader(const std::string &filename) {
    _ = std::make_unique<RawGridVolumeReaderPrivate>();

    if(!_->file.Open(filename)){
        throw VolumeFileOpenError("RawGridVolumeFile open failed : " + filename);
    }

    _->desc = _->file.GetVolumeDesc();
    if(!CheckValidation(_->desc)){
        PrintVolumeDesc(_->desc);
        throw VolumeFileContextError("RawGridVolumeFile context is not right : " + filename);
    }

    _->in.open(_->file.GetDataPath(), std::ios::binary);
    if(!_->in.is_open()){
        throw VolumeFileOpenError("Failed to open raw volume file : " + _->file.GetDataPath());
    }
}

RawGridVolumeReader::~RawGridVolumeReader() {

}

void RawGridVolumeReader::ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, void *buf) {
    assert(buf && srcX < dstX && srcY < dstY && srcZ < dstZ);

#ifndef HIGH_PERFORMANCE
    size_t voxel_size = GetVoxelSize(_->desc.voxel_info);
    auto copy_func = GetCopyBitsFunc(voxel_size);
    return ReadVolumeData(srcX, srcY, srcZ, dstX, dstY, dstZ,
                          [&copy_func, width = dstX - srcX, height = dstY - srcY,
                           dst_ptr = reinterpret_cast<uint8_t*>(buf)]
                          (int dx, int dy, int dz, const void* src, size_t ele_size){
        size_t dst_offset = ((size_t)dz * width * height + (size_t)dy * width + dx) * ele_size;
        copy_func(reinterpret_cast<const uint8_t*>(src), dst_ptr + dst_offset);
    });
#else
    auto width = _->desc.extend.width;
    auto height = _->desc.extend.height;
    auto depth = _->desc.extend.depth;

    int beg_x = std::max<int>(0, srcX), end_x = std::min<int>(dstX, width);
    int beg_y = std::max<int>(0, srcY), end_y = std::min<int>(dstY, height);
    int beg_z = std::max<int>(0, srcZ), end_z = std::min<int>(dstZ, depth);
    size_t voxel_size = GetVoxelSize(_->desc.voxel_info);
    size_t x_voxel_size = voxel_size * (end_x - beg_x);
    std::vector<uint8_t> voxel(x_voxel_size, 0);
    for(int z = beg_z; z < end_z; z++){
        for(int y = beg_y; y < end_y; y++){
            size_t dst_offset = ((size_t)(z - srcZ) * (dstX - srcX) * (dstY - srcY) + (size_t)(y - srcY) * (dstX - srcX) + beg_x - srcX) * voxel_size;
            size_t src_offset = ((size_t)z * width * height + y * width + beg_x) * voxel_size;
            _->in.seekg(src_offset, std::ios::beg);
            _->in.read(reinterpret_cast<char*>(voxel.data()), x_voxel_size);
            std::memcpy(reinterpret_cast<uint8_t*>(buf) + dst_offset, voxel.data(), x_voxel_size);
        }
    }
#endif
}

void RawGridVolumeReader::ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadFunc reader) {
    assert(reader && srcX < dstX && srcY < dstY && srcZ < dstZ);

    auto width = _->desc.extend.width;
    auto height = _->desc.extend.height;
    auto depth = _->desc.extend.depth;

    int beg_x = std::max<int>(0, srcX), end_x = std::min<int>(dstX, width);
    int beg_y = std::max<int>(0, srcY), end_y = std::min<int>(dstY, height);
    int beg_z = std::max<int>(0, srcZ), end_z = std::min<int>(dstZ, depth);
    auto voxel_size = GetVoxelSize(_->desc.voxel_info);
    auto x_voxel_size = voxel_size * (end_x - beg_x);
    std::vector<uint8_t> voxel(x_voxel_size, 0);
    for(int z = beg_z; z < end_z; z++){
        for(int y = beg_y; y < end_y; y++){
            size_t src_offset_beg = ((size_t)z * width * height + (size_t)y * width + beg_x) * voxel_size;
            _->in.seekg(src_offset_beg, std::ios::beg);
            _->in.read(reinterpret_cast<char*>(voxel.data()), x_voxel_size);
            for(int x = beg_x; x < end_x; x++){
                reader(x - srcX, y - srcY, z - srcZ, voxel.data() + x * voxel_size, voxel_size);
            }
        }
    }
}

RawGridVolumeDesc RawGridVolumeReader::GetVolumeDesc() const noexcept {
    return _->desc;
}

class RawGridVolumeWriterPrivate{
public:
#ifndef USE_MAPPING_FILE
    RawGridVolumeDesc desc;
    RawGridVolumeFile file;
    std::ofstream out;
#else

#endif
};


RawGridVolumeWriter::RawGridVolumeWriter(const std::string &filename, const RawGridVolumeDesc& desc) {
    _ = std::make_unique<RawGridVolumeWriterPrivate>();

    if(!CheckValidation(desc)){
        PrintVolumeDesc(desc);
        throw VolumeFileContextError("RawGridVolumeFile context wrong : " + filename);
    }
    _->desc = desc;

    if(!_->file.Save(filename, desc)){
        throw VolumeFileOpenError("RawGridVolume file save failed : " + filename);
    }

    _->out.open(_->file.GetDataPath(), std::ios::binary);
    if(!_->out.is_open()){
        throw std::runtime_error("Failed to open raw volume file : " + _->file.GetDataPath());
    }
}

RawGridVolumeWriter::~RawGridVolumeWriter() {

}

RawGridVolumeDesc RawGridVolumeWriter::GetVolumeDesc() const noexcept {
    return _->desc;
}

void RawGridVolumeWriter::WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const void *buf) {
    assert(buf && srcX < dstX && srcY < dstY && srcZ < dstZ);

#ifndef HIGH_PERFORMANCE
    size_t voxel_size = GetVoxelSize(_->desc.voxel_info);
    auto copy_func = GetCopyBitsFunc(voxel_size);
    return WriteVolumeData(srcX, srcY, srcZ, dstX, dstY, dstZ,
                           [&copy_func, width = dstX - srcX, height = dstY - srcY,
                            src_ptr = reinterpret_cast<const uint8_t*>(buf)]
                           (int dx, int dy, int dz, void* dst, size_t ele_size){
        size_t src_offset = ((size_t)dz * width * height + (size_t)dy * width + dx) * ele_size;
        copy_func(src_ptr + src_offset, reinterpret_cast<uint8_t*>(dst));
    });
#else
    auto width = _->desc.extend.width;
    auto height = _->desc.extend.height;
    auto depth = _->desc.extend.depth;

    int beg_x = std::max<int>(0, srcX), end_x = std::min<int>(dstX, width);
    int beg_y = std::max<int>(0, srcY), end_y = std::min<int>(dstY, height);
    int beg_z = std::max<int>(0, srcZ), end_z = std::min<int>(dstZ, depth);
    auto voxel_size = GetVoxelSize(_->desc.voxel_info);
    auto x_voxel_size = voxel_size * (end_x - beg_x);
    std::vector<uint8_t> voxel(x_voxel_size, 0);
    for(int z = beg_z; z < end_z; z++){
        for(int y = beg_y; y < end_y; y++){
            size_t dst_offset = ((size_t)z * width * height + (size_t)y * width + beg_x) * voxel_size;
            size_t src_offset = ((size_t)(z - srcZ) * (dstY - srcY) * (dstX - srcX) + (size_t)(y - srcY) * (dstX - srcX) + beg_x - srcX) * voxel_size;
            std::memcpy(voxel.data(), reinterpret_cast<const uint8_t*>(buf)+ src_offset, x_voxel_size);
            _->out.seekp(dst_offset, std::ios::beg);
            _->out.write(reinterpret_cast<char*>(voxel.data()), x_voxel_size);
        }
    }
#endif
}

void RawGridVolumeWriter::WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeWriteFunc writer) {
    assert(writer && srcX < dstX && srcY < dstY && srcZ < dstZ);

    auto width = _->desc.extend.width;
    auto height = _->desc.extend.height;
    auto depth = _->desc.extend.depth;

    int beg_x = std::max<int>(0, srcX), end_x = std::min<int>(dstX, width);
    int beg_y = std::max<int>(0, srcY), end_y = std::min<int>(dstY, height);
    int beg_z = std::max<int>(0, srcZ), end_z = std::min<int>(dstZ, depth);
    auto voxel_size = GetVoxelSize(_->desc.voxel_info);
    auto x_voxel_size = voxel_size * (end_x - beg_x);
    std::vector<uint8_t> voxel(x_voxel_size, 0);
    for(int z = beg_z; z < end_z; z++){
        for(int y = beg_y; y < end_y; y++){
            for(int x = beg_x; x < end_x; x++){
                writer(x - srcX, y - srcY, z - srcZ, voxel.data() + x * voxel_size, voxel_size);
            }
            size_t dst_offset_beg = ((size_t)z * width * height + (size_t)y * width + beg_x) * voxel_size;
            _->out.seekp(dst_offset_beg, std::ios::beg);
            _->out.write(reinterpret_cast<char*>(voxel.data()), x_voxel_size);
        }
    }
}

VOL_END