//
// Created by wyz on 2022/4/28.
//

#ifndef VOLUMEUTILS_VOLUME_HPP
#define VOLUMEUTILS_VOLUME_HPP

#include <string>
#include <functional>
#include <vector>
#include <memory>
enum class VoxelType {
    unknown = 0, uint8, uint16, float32
};
enum class VoxelFormat {
    NONE = 0, R = 1, RG = 2, RGB = 3, RGBA = 4
};
struct VoxelSpace {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
};
template<VoxelType type, VoxelFormat format>
struct Voxel;

template<>
struct Voxel<VoxelType::uint8, VoxelFormat::R> {
    union {
        uint8_t x = 0;
        uint8_t r;
    };

    static constexpr size_t size() { return 1; }
};

template<>
struct Voxel<VoxelType::uint16, VoxelFormat::R> {
    union {
        uint16_t x = 0;
        uint16_t r;
    };

    static constexpr size_t size() { return 2; }
};

struct Extend3D {
    uint32_t width;
    uint32_t height;
    uint32_t depth;

    size_t size() const {
        return static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(depth);
    }
};
struct GridVolumeDesc {
    Extend3D extend;
    VoxelSpace space;
};

class GridVolumePrivate;
template<typename Voxel>
class GridVolume {
public:

    GridVolume(const GridVolumeDesc &desc, const std::string filename);

    ~GridVolume();

    const GridVolumeDesc &GetGridVolumeDesc() const;

    //xyz may out of volume is acceptable, but length of buf must equal to dx*dy*dz
    size_t ReadVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, Voxel *buf);

    void UpdateVoxels(uint32_t srcX, uint32_t srcY, uint32_t srcZ, uint32_t dstX, uint32_t dstY, uint32_t dstZ, const Voxel *buf);

    Voxel& operator()(int x,int y,int z);

    Voxel* GetData();

private:
    std::unique_ptr<GridVolumePrivate> _;
};

struct SlicedGridVolumeDesc {
    Extend3D extend;
    VoxelSpace space;
    std::function<std::string(uint32_t)> name_generator;
};

template<typename Voxel>
class SlicedGridVolume {
public:


    SlicedGridVolume(const SlicedGridVolumeDesc &desc, const std::string filename);

    ~SlicedGridVolume();

    const SlicedGridVolumeDesc &GetSlicedGridVolumeDesc() const;

    void ReadSlice(uint32_t z, Voxel *buf);

    size_t ReadVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, Voxel *buf);


    void UpdateVoxels(uint32_t srcX, uint32_t srcY, uint32_t srcZ, uint32_t dstX, uint32_t dstY, uint32_t dstZ,
                      const Voxel *buf);

};

enum class GridVolumeDataCodec {

};

using Packet = std::vector<uint8_t>;
using Packets = std::vector<Packet>;
struct BlockedGridVolumeDesc {
    uint32_t block_length;
    uint32_t padding;
    Extend3D extend;
    VoxelSpace space;
};
struct BlockIndex {
    uint32_t x;
    uint32_t y;
    uint32_t z;
};
template<typename Voxel>
class BlockedGridVolume {
public:


    BlockedGridVolume(const std::string &filename);

    ~BlockedGridVolume();

    const BlockedGridVolumeDesc &GetBlockedGridVolumeDesc() const;

    void ReadBlockData(const BlockIndex &, std::vector<Voxel> &);

    void ReadBlockData(const BlockIndex &, Voxel *buf);

    void WriteBlockData(const BlockIndex &, const std::vector<Voxel> &);

    void WriteBlockData(const BlockIndex &, const Voxel *buf);


    void ReadRawBlockData(const BlockIndex &, Packet &);

    void ReadRawBlockData(const BlockIndex &, Packets &);

    void WriteRawBlockData(const BlockIndex &, const Packet &);

    void WriteRawBlockData(const BlockIndex &, const Packets &);
};

template<typename T>
class SliceDataView {
public:
    SliceDataView(uint32_t sizeX, uint32_t sizeY, T *srcP = nullptr);


    const T *GetData();
};



template<typename T>
class GridVolumeDataView {
public:
    GridVolumeDataView(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, T *srcP = nullptr);

    SliceDataView<T> ViewSliceX(int x);

    SliceDataView<T> ViewSliceY(int y);

    SliceDataView<T> ViewSliceZ(int z);

    T At(int x, int y, int z) const;

    const T *GetData();
};

enum class CodecDevice {
    CPU, GPU
};

template<typename T, CodecDevice device>
class VideoCodec {
public:
    static bool Encode(const std::vector<SliceDataView<T>> &slices, Packet &packet);

    static bool Encode(const std::vector<SliceDataView<T>> &slices, Packets &packets);

    static bool Decode(const Packet &packet, void* buf);

    static bool Decode(const Packets &packets, void* buf);

};


#endif // VOLUMEUTILS_VOLUME_HPP
