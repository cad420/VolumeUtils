#pragma once
#include <VolumeUtils/Volume.hpp>
#include "Utils.hpp"
#include <iostream>
VOL_BEGIN

inline VoxelType StrToVoxelType(std::string_view str){
    auto s = ConvertStrToLower(str);
    // lazy method
    if(s == "uint8") return VoxelType::uint8;
    if(s == "uint16") return VoxelType::uint16;
    if(s == "float32") return VoxelType::float32;
    return VoxelType::unknown;
}

inline VoxelFormat StrToVoxelFormat(std::string_view str){
    auto s = ConvertStrToLower(str);
    if(s == "r") return VoxelFormat::R;
    if(s == "rg") return VoxelFormat::RG;
    if(s == "rgb") return VoxelFormat::RGB;
    if(s == "rgba") return VoxelFormat::RGBA;
    return VoxelFormat::NONE;
}

inline constexpr const char* VoxelTypeToStr(VoxelType type){
    switch (type) {
        case VoxelType::uint8 : return "uint8";
        case VoxelType::uint16 : return "uint16";
        case VoxelType::float32 : return "float32";
        default : return "unknown";
    }
}

inline constexpr const char* VoxelFormatToStr(VoxelFormat format){
    switch (format) {
        case VoxelFormat::R : return "R";
        case VoxelFormat::RG : return "RG";
        case VoxelFormat::RGB : return "RGB";
        case VoxelFormat::RGBA : return "RGBA";
        default : return "NONE";
    }
}

inline void PrintVolumeDesc(const VolumeDesc& desc, bool printName = true){
    if(printName){
        std::cerr << "VolumeDesc Info : " << std::endl;
    }
    std::cerr << "\tvolume_name : " << desc.volume_name << std::endl;
    std::cerr << "\tvoxel_info : " << "type( " << VoxelTypeToStr(desc.voxel_info.type) << " ), "
                                   << "format( " << VoxelFormatToStr(desc.voxel_info.format) << " )." << std::endl;
}

inline void PrintVolumeDesc(const RawGridVolumeDesc& desc, bool printName = true){
    if(printName){
        std::cerr << "RawGridVolumeDesc Info : " << std::endl;
    }
    PrintVolumeDesc(static_cast<const VolumeDesc&>(desc), false);
    std::cerr << "\textend : " << desc.extend << std::endl;
    std::cerr << "\tspace : " << desc.space << std::endl;
}

inline void PrintVolumeDesc(const SlicedGridVolumeDesc& desc, bool printName = true){
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

inline void PrintVolumeDesc(const BlockedGridVolumeDesc& desc, bool printName = true){

}

inline void PrintVolumeDesc(const EncodedGridVolumeDesc& desc, bool printName = true){

}

inline void PrintVolumeDesc(const EncodedBlockedGridVolumeDesc& desc, bool printName = true){

}


VOL_END