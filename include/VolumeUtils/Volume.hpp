//
// Created by wyz on 2022/4/28.
//

#ifndef VOLUMEUTILS_VOLUME_HPP
#define VOLUMEUTILS_VOLUME_HPP

#include <string>
#include <functional>
#include <vector>
#include <memory>

enum class VoxelType :int{
    unknown = 0, uint8, uint16, float32
};
enum class VoxelFormat :int{
    NONE = 0, R = 1, RG = 2, RGB = 3, RGBA = 4
};
inline size_t GetVoxelSize(VoxelType type,VoxelFormat format){
    size_t size = 0;
    switch (type) {
        case VoxelType::uint8:size = 1;
            break;
        case VoxelType::uint16:size = 2;
            break;
        case VoxelType::float32:size = 4;
            break;
        default:
            break;
    }
    switch (format) {
        case VoxelFormat::R: size *= 1;
            break;
        case VoxelFormat::RG: size *= 2;
            break;
        case VoxelFormat::RGB: size *= 3;
            break;
        case VoxelFormat::RGBA: size *= 4;
            break;
        default:
            break;
    }
    return size;
}
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
using VoxelRU8 = Voxel<VoxelType::uint8,VoxelFormat::R>;

template<>
struct Voxel<VoxelType::uint16, VoxelFormat::R> {
    union {
        uint16_t x = 0;
        uint16_t r;
    };

    static constexpr size_t size() { return 2; }
};
using VoxelRU16 = Voxel<VoxelType::uint16,VoxelFormat::R>;

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


template<typename Voxel>
class VolumeInterface{
public:
    virtual size_t ReadVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, Voxel *buf) = 0;

    virtual void UpdateVoxels(uint32_t srcX, uint32_t srcY, uint32_t srcZ, uint32_t dstX, uint32_t dstY, uint32_t dstZ, const Voxel *buf) = 0;

};

class GridVolumePrivate;
template<typename Voxel>
class GridVolume : public VolumeInterface<Voxel>{
public:
    using VoxelT = Voxel;
    GridVolume(const GridVolumeDesc &desc, const std::string& filename);

    ~GridVolume();

    const GridVolumeDesc &GetGridVolumeDesc() const;

    //xyz may out of volume is acceptable, but length of buf must equal to dx*dy*dz
    size_t ReadVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, Voxel *buf) override;

    void UpdateVoxels(uint32_t srcX, uint32_t srcY, uint32_t srcZ, uint32_t dstX, uint32_t dstY, uint32_t dstZ, const Voxel *buf) override;

    Voxel& operator()(int x,int y,int z);

    Voxel* GetData();

private:
    std::unique_ptr<GridVolumePrivate> _;
};
enum class SliceAxis{
    AXIS_X,AXIS_Y,AXIS_Z
};
struct SlicedGridVolumeDesc {
    GridVolumeDesc desc;
    SliceAxis axis;
    std::function<std::string(uint32_t)> name_generator;
};

template<typename Voxel>
class SlicedGridVolume :public VolumeInterface<Voxel>{
public:
    using VoxelT = Voxel;
    SlicedGridVolume(const SlicedGridVolumeDesc &desc, const std::string filename);

    ~SlicedGridVolume();

    const SlicedGridVolumeDesc &GetSlicedGridVolumeDesc() const;

    void ReadSlice(uint32_t z, Voxel *buf);

    size_t ReadVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, Voxel *buf) override;


    void UpdateVoxels(uint32_t srcX, uint32_t srcY, uint32_t srcZ, uint32_t dstX, uint32_t dstY, uint32_t dstZ,
                      const Voxel *buf) override;

};
struct BlockIndex {
    uint32_t x;
    uint32_t y;
    uint32_t z;
    bool operator==(const BlockIndex& blockIndex) const{
        return x == blockIndex.x && y == blockIndex.y && z == blockIndex.z;
    }
};
template<>
struct std::hash<BlockIndex>{
    size_t operator()(const BlockIndex& blockIndex) const{
        auto a = std::hash<uint32_t>()(blockIndex.x);
        auto b = std::hash<uint32_t>()(blockIndex.y);
        auto c = std::hash<uint32_t>()(blockIndex.z);
        return a ^ (b + 0x9e3779b97f4a7c15LL + (c << 6) + (c >> 2));
    }
};
struct BlockedGridVolumeDesc {
    uint32_t block_length;
    uint32_t padding;
    Extend3D extend;
    VoxelSpace space;
};
template <typename GridVolume>
class BlockGridVolume:public GridVolume{
public:
    BlockGridVolume(const BlockedGridVolumeDesc&,const std::string);
    void ReadBlock(const BlockIndex&,typename GridVolume::VoxelT* buf);
    void WriteBlock(const BlockIndex&,const typename GridVolume::VoxelT* buf);
};

enum class GridVolumeDataCodec:int {
    GRID_VOLUME_CODEC_NONE = 0,
    GRID_VOLUME_CODEC_VIDEO = 1,
    GRID_VOLUME_CODEC_BYTES = 2
};

using Packet = std::vector<uint8_t>;
using Packets = std::vector<Packet>;

struct EncodedBlockedGridVolumeDesc {
    uint32_t block_length;
    uint32_t padding;
    Extend3D extend;
    VoxelSpace space;
    GridVolumeDataCodec codec;
    VoxelType type;
    VoxelFormat format;
    char preserve[20];
};//64
inline constexpr int EncodedBlockedGridVolumeDescSize = 64;
static_assert(sizeof(EncodedBlockedGridVolumeDesc) == EncodedBlockedGridVolumeDescSize,"");

inline bool CheckValidation(const EncodedBlockedGridVolumeDesc& desc){
    if(desc.block_length == 0 || desc.padding == 0 || desc.block_length <= (desc.padding << 1)){
        return false;
    }
    if(desc.extend.width == 0 || desc.extend.height == 0 || desc.extend.depth == 0){
        return false;
    }
    return true;
}

class EncodedBlockedGridVolumeReaderPrivate;
class EncodedBlockedGridVolumeReader {
public:
    explicit EncodedBlockedGridVolumeReader(const std::string &filename);

    ~EncodedBlockedGridVolumeReader();

    void Close();

    const EncodedBlockedGridVolumeDesc &GetEncodedBlockedGridVolumeDesc() const;

    void ReadBlockData(const BlockIndex &, void* buf);

    void ReadRawBlockData(const BlockIndex &, Packet &);

    void ReadRawBlockData(const BlockIndex &, Packets &);
private:
    std::unique_ptr<EncodedBlockedGridVolumeReaderPrivate> _;
};

class EncodedBlockedGridVolumeWriterPrivate;
class EncodedBlockedGridVolumeWriter {
public:
    EncodedBlockedGridVolumeWriter(const std::string &filename,const EncodedBlockedGridVolumeDesc& desc);

    ~EncodedBlockedGridVolumeWriter();

    const EncodedBlockedGridVolumeDesc &GetEncodedBlockedGridVolumeDesc() const;

    void Close();

    void WriteBlockData(const BlockIndex &, const void *buf);

    void WriteRawBlockData(const BlockIndex &, const Packet &);

    void WriteRawBlockData(const BlockIndex &, const Packets &);
private:
    std::unique_ptr<EncodedBlockedGridVolumeWriterPrivate> _;
};

template<typename T>
class SliceDataView {
public:
    SliceDataView(uint32_t sizeX, uint32_t sizeY, T *srcP = nullptr);

    const T *GetData();
};

template<typename T>
class GridDataView {
public:
    GridDataView(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, T *srcP = nullptr);

    SliceDataView<T> ViewSliceX(int x);

    SliceDataView<T> ViewSliceY(int y);

    SliceDataView<T> ViewSliceZ(int z);

    T At(int x, int y, int z) const;

    const T *GetData();
};

enum class CodecDevice {
    CPU, GPU
};
template<typename Voxel>
struct VideoCodecVoxel{
    static constexpr bool value = false;
};
#define VIDEO_CODEC_VOXEL_REGISTER(VoxelT) \
template<>\
struct VideoCodecVoxel<VoxelT>{\
    static constexpr bool value = true;        \
};

VIDEO_CODEC_VOXEL_REGISTER(VoxelRU8)
VIDEO_CODEC_VOXEL_REGISTER(VoxelRU16)

template<typename T>
inline constexpr bool VideoCodecVoxelV = VideoCodecVoxel<T>::value;

template<typename T,CodecDevice device,typename V = void>
class VideoCodec;

template<typename T>
class VideoCodec<T,CodecDevice::CPU,
        std::enable_if_t<VideoCodecVoxelV<T>>
        >{
public:
    VideoCodec() = delete;
    inline static int MaxThreads = 8;
    static bool Encode(const std::vector<SliceDataView<T>> &slices, Packet &packet,int maxThreads = MaxThreads);
    static bool Encode(const GridDataView<T>& volume,SliceAxis axis, Packet &packet,int maxThreads = MaxThreads);
    static bool Encode(const std::vector<SliceDataView<T>> &slices, Packets &packets,int maxThreads = MaxThreads);
    static bool Encode(const GridDataView<T>& volume,SliceAxis axis, Packets &packets,int maxThreads = MaxThreads);

    static bool Decode(const Packet &packet, void* buf,int maxThreads = MaxThreads);

    static bool Decode(const Packets &packets, void* buf,int maxThreads = MaxThreads);
};

template<typename T>
class VideoCodec<T,CodecDevice::GPU,
        std::enable_if_t<VideoCodecVoxelV<T>>
        >{
public:
    VideoCodec(int GPUIndex);
    VideoCodec(void* context);
    bool Encode(const std::vector<SliceDataView<T>> &slices, Packet &packet);
    bool Encode(const GridDataView<T>& volume,SliceAxis axis, Packet &packet);
    bool Encode(const std::vector<SliceDataView<T>> &slices, Packets &packets);
    bool Encode(const GridDataView<T>& volume,SliceAxis axis, Packets &packets);

    bool Decode(const Packet &packet, void* buf);

    bool Decode(const Packets &packets, void* buf);
};







#endif // VOLUMEUTILS_VOLUME_HPP
