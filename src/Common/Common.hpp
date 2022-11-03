#pragma once

#include <VolumeUtils/Volume.hpp>
#include <json.hpp>
#include "Utils.hpp"

#include <iostream>
#include <fstream>

VOL_BEGIN

inline VoxelType StrToVoxelType(std::string_view str) noexcept {
    auto s = ConvertStrToLower(str);
    // lazy method
    if(s == "uint8")   return VoxelType::uint8;
    if(s == "uint16")  return VoxelType::uint16;
    if(s == "float32") return VoxelType::float32;
    return VoxelType::unknown;
}

inline GridVolumeCodec StrToGridVolumeDataCodec(std::string_view str) noexcept {
    auto s = ConvertStrToLower(str);
    if(s == "video") return GridVolumeCodec::GRID_VOLUME_CODEC_VIDEO;
    if(s == "bits")  return GridVolumeCodec::GRID_VOLUME_CODEC_BITS;
    return GridVolumeCodec::GRID_VOLUME_CODEC_NONE;
}

inline VoxelFormat StrToVoxelFormat(std::string_view str) noexcept {
    auto s = ConvertStrToLower(str);
    if(s == "r")    return VoxelFormat::R;
    if(s == "rg")   return VoxelFormat::RG;
    if(s == "rgb")  return VoxelFormat::RGB;
    if(s == "rgba") return VoxelFormat::RGBA;
    return VoxelFormat::NONE;
}

inline constexpr const char* VoxelTypeToStr(VoxelType type) noexcept {
    switch (type) {
        case VoxelType::uint8 :   return "uint8";
        case VoxelType::uint16 :  return "uint16";
        case VoxelType::float32 : return "float32";
        default : return "unknown";
    }
}

inline constexpr const char* VoxelFormatToStr(VoxelFormat format) noexcept {
    switch (format) {
        case VoxelFormat::R :    return "r";
        case VoxelFormat::RG :   return "rg";
        case VoxelFormat::RGB :  return "rgb";
        case VoxelFormat::RGBA : return "rgba";
        default : return "none";
    }
}

inline constexpr const char* VolumeCodecToStr(GridVolumeCodec codec) noexcept {
    switch (codec) {
        case GridVolumeCodec::GRID_VOLUME_CODEC_BITS :  return "bits";
        case GridVolumeCodec::GRID_VOLUME_CODEC_VIDEO : return "video";
        default : return "none";
    }
}

inline void PrintVolumeDesc(const VolumeDesc& desc, bool printName = true) noexcept {
    if(printName){
        std::cerr << "VolumeDesc Info : " << std::endl;
    }
    std::cerr << "\tvolume_name : " << desc.volume_name << std::endl;
    std::cerr << "\tvoxel_info : " << "type( " << VoxelTypeToStr(desc.voxel_info.type) << " ), "
                                   << "format( " << VoxelFormatToStr(desc.voxel_info.format) << " )."
                                   << std::endl;
}

inline void PrintVolumeDesc(const RawGridVolumeDesc& desc, bool printName = true) noexcept {
    if(printName){
        std::cerr << "RawGridVolumeDesc Info : " << std::endl;
    }
    PrintVolumeDesc(static_cast<const VolumeDesc&>(desc), false);
    std::cerr << "\textend : " << desc.extend << std::endl;
    std::cerr << "\tspace : " << desc.space << std::endl;
}

inline void PrintVolumeDesc(const SlicedGridVolumeDesc& desc, bool printName = true) noexcept {
    if(printName){
        std::cerr << "SlicedGridVolumeDesc Info : " << std::endl;
    }
    PrintVolumeDesc(static_cast<const RawGridVolumeDesc&>(desc), false);
    std::cerr << "\taxis : " << static_cast<int>(desc.axis) << std::endl;
    std::cerr << "\tname_generator : " << (desc.name_generator ? "have" : "none") << std::endl;
    std::cerr << "\tprefix : " << desc.prefix << std::endl;
    std::cerr << "\tpostfix : " << desc.postfix << std::endl;
    std::cerr << "\tsetw : " << desc.setw << std::endl;
}

inline void PrintVolumeDesc(const BlockedGridVolumeDesc& desc, bool printName = true) noexcept {
    if(printName){
        std::cerr << "BlockedGridVolumeDesc Info : " << std::endl;
    }
    PrintVolumeDesc(static_cast<const RawGridVolumeDesc&>(desc), false);
    std::cerr << "\t block_length : " << desc.block_length << std::endl;
    std::cerr << "\t padding : " << desc.padding << std::endl;
}

inline void PrintVolumeDesc(const EncodedGridVolumeDesc& desc, bool printName = true) noexcept {
    if(printName){
        std::cerr << "EncodedGridVolumeDesc Info : " << std::endl;
    }
    PrintVolumeDesc(static_cast<const RawGridVolumeDesc&>(desc), false);
    std::cerr << "\tcodec : " << VolumeCodecToStr(desc.codec) << std::endl;
}

inline void PrintVolumeDesc(const EncodedBlockedGridVolumeDesc& desc, bool printName = true) noexcept {
    if(printName){
        std::cerr << "EncodedBlockedGridVolumeDesc Info : " << std::endl;
    }
    PrintVolumeDesc(static_cast<const BlockedGridVolumeDesc&>(desc), false);
    std::cerr << "\tcodec : " << VolumeCodecToStr(desc.codec) << std::endl;
}

inline auto CreateCPUVolumeVideoCodecByVoxel(const VoxelInfo& voxel_info)->std::unique_ptr<CVolumeCodecInterface>{
    auto [type, format] = voxel_info;
    if(type == VoxelType::uint8){
        if(format == VoxelFormat::R){
            return std::make_unique<VolumeVideoCodec<VoxelRU8,CodecDevice::CPU>>(1);
        }
    }
    else if(type == VoxelType::uint16){
        if(format == VoxelFormat::R){
            return std::make_unique<VolumeVideoCodec<VoxelRU16,CodecDevice::CPU>>(1);
        }
    }
    return nullptr;
}
namespace detail{
    inline const char* raw          = "raw";
    inline const char* sliced       = "sliced";
    inline const char* slice_format = "slice_format";
    inline const char* encoded_blocked = "encoded_blocked";
    inline const char* volume_name = "volume_name";
    inline const char* voxel_type = "voxel_type";
    inline const char* voxel_format = "voxel_format";
    inline const char* extend = "extend";
    inline const char* space = "space";
    inline const char* block_length = "block_length";
    inline const char* padding = "padding";
    inline const char* volume_codec = "volume_codec";
    inline const char* data_path = "data_path";
    inline const char* axis         = "axis";
    inline const char* prefix       = "prefix";
    inline const char* postfix      = "postfix";
    inline const char* setw         = "setw";
}
template<typename Json>
void ReadDescFromJson(EncodedBlockedGridVolumeDesc& desc, Json& encoded_block){
    using namespace detail;
    desc.volume_name = encoded_block.count(volume_name) == 0 ? "none" : std::string(encoded_block.at(volume_name));
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
    desc.block_length = encoded_block.count(block_length) == 0 ? 0 : (int)encoded_block.at(block_length);
    desc.padding = encoded_block.count(padding) == 0 ? 0 : (int)encoded_block.at(padding);
    desc.codec = StrToGridVolumeDataCodec(encoded_block.count(volume_codec) == 0 ? "none" : encoded_block.at(volume_codec));

    desc.data_path = encoded_block.count(data_path) == 0 ? "none" : std::string(encoded_block.at(data_path));

}

template<typename Json>
void ReadDescFromJson(RawGridVolumeDesc& desc, Json& raw_){
    using namespace detail;
    desc.volume_name = raw_.count(volume_name) == 0 ? "none" : std::string(raw_.at(volume_name));
    desc.voxel_info.type = StrToVoxelType(raw_.count(voxel_type) == 0 ? "unknown" : raw_.at(voxel_type));
    desc.voxel_info.format = StrToVoxelFormat(raw_.count(voxel_format) == 0 ? "none" : raw_.at(voxel_format));
    if(raw_.count(extend) != 0){
        std::array<int, 3> shape = raw_.at(extend);
        desc.extend = {(uint32_t)shape[0], (uint32_t)shape[1], (uint32_t)shape[2]};
    }
    if(raw_.count(space) != 0){
        std::array<float, 3> sp = raw_.at(space);
        desc.space = {sp[0], sp[1], sp[2]};
    }
    desc.data_path = raw_.count(data_path) == 0 ? "" : std::string(raw_.at(data_path));

}

template<typename Json>
void ReadDescFromJson(SlicedGridVolumeDesc& desc, Json& slice){
    using namespace detail;

    desc.volume_name = slice.count(volume_name) == 0 ? "none" : std::string(slice.at(volume_name));
    desc.voxel_info.type = StrToVoxelType(slice.count(voxel_type) == 0 ? "unknown" : slice.at(voxel_type));
    desc.voxel_info.format = StrToVoxelFormat(slice.count(voxel_format) == 0 ? "none" : slice.at(voxel_format));
    if(slice.count(extend) != 0){
        std::array<int, 3> shape = slice.at(extend);
        desc.extend = {(uint32_t)shape[0], (uint32_t)shape[1], (uint32_t)shape[2]};
    }
    if(slice.count(space) != 0){
        std::array<float, 3> sp = slice.at(space);
        desc.space = {sp[0], sp[1], sp[2]};
    }
    desc.axis = slice.count(axis) == 0 ? SliceAxis::AXIS_Z : static_cast<SliceAxis>(slice.at(axis));
    desc.prefix = slice.count(prefix) == 0 ? "" : slice.at(prefix);
    desc.postfix = slice.count(postfix) == 0 ? "" : slice.at(postfix);
    desc.setw = slice.count(setw) == 0 ? 0 : (int)slice.at(setw);
}

template<typename VolumeDesc>
void ReadDescFromFile(VolumeDesc& desc, const std::string& filename){

    std::ifstream io(filename, std::ios::in);
    if(!io.is_open()){
        throw VolumeFileOpenError("Failed to open eb desc file : " + filename);
    }

    nlohmann::json j;
    io >> j;
    if(j.count("desc") == 0){
        throw VolumeFileContextError("Desc file wrong context : " + filename);
    }

    ReadDescFromJson(desc, j.at("desc"));
}

VOL_END