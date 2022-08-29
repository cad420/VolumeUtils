#pragma once

#define VOL_BEGIN namespace vol{
#define VOL_END }
// DEBUG
#ifndef NDEBUG
#define VOL_DEBUG
#define VOL_WHEN_DEBUG(op) do{op;}while(false);
#else
#define VOL_WHEN_DEBUG(op) do{}while(false);
#endif
// CXX
#if defined(_MSC_VER)
#define VOL_CXX_MSVC
#elif defined(__clang__)
#define VOL_CXX_CLANG
#define VOL_CXX_IS_GNU
#elif defined(__GNUC__)
#define VOL_CXX_GCC
#define VOL_CXX_IS_GNU
#endif


#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <cassert>

VOL_BEGIN

#include "Volume.inl"

enum class VoxelType :int{
    unknown = 0, uint8, uint16, float32
};

enum class VoxelFormat :int{
    NONE = 0, R = 1, RG = 2, RGB = 3, RGBA = 4
};

inline constexpr int GetVoxelSampleCount(VoxelFormat format) {
    int nSamples = 0;
    switch (format) {
        case VoxelFormat::R:
            nSamples = 1;
            break;
        case VoxelFormat::RG:
            nSamples = 2;
            break;
        case VoxelFormat::RGB:
            nSamples = 3;
            break;
        case VoxelFormat::RGBA:
            nSamples = 4;
            break;
        default:
            nSamples = 0;
    }
    return nSamples;
}

inline constexpr int GetVoxelBits(VoxelType type) {
    int bits = 0;
    switch (type) {
        case VoxelType::uint8:
            bits = 8;
            break;
        case VoxelType::uint16:
            bits = 16;
            break;
        case VoxelType::float32:
            bits = 32;
            break;
        default:
            bits = 0;
    }
    return bits;
}

inline constexpr size_t GetVoxelSize(VoxelType type, VoxelFormat format) {
    size_t size = 0;
    switch (type) {
        case VoxelType::uint8:
            size = 1;
            break;
        case VoxelType::uint16:
            size = 2;
            break;
        case VoxelType::float32:
            size = 4;
            break;
        default:
            break;
    }
    switch (format) {
        case VoxelFormat::R:
            size *= 1;
            break;
        case VoxelFormat::RG:
            size *= 2;
            break;
        case VoxelFormat::RGB:
            size *= 3;
            break;
        case VoxelFormat::RGBA:
            size *= 4;
            break;
        default:
            break;
    }
    return size;
}

struct VoxelSpace {
    float x = 1.f;
    float y = 1.f;
    float z = 1.f;
};

struct VoxelInfo{
    VoxelType type;
    VoxelFormat format;
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



//histogram
template<typename Voxel>
class VolumeStatistics{
public:

};

enum class VolumeType{
    Grid_RAW,
    Grid_SLICED,
    Grid_BLOCKED_ENCODED,
    Grid_ENCODED,
    Grid_BLOCKED,
};

template <typename Voxel>
class RawGridVolume;
class RawGridVolumeReader;
class RawGridVolumeWriter;

template <typename Voxel>
class EncodedGridVolume;
class EncodedGridVolumeReader;
class EncodedGridVolumeWriter;

template<typename Voxel>
class SlicedGridVolume;
class SlicedGridVolumeReader;
class SlicedGridVolumeWriter;

template <typename Voxel>
class BlockedGridVolume;
class BlockedGridVolumeReader;
class BlockedGridVolumeWriter;

template<typename Voxel>
class EncodedBlockedGridVolume;
class EncodedBlockedGridVolumeReader;
class EncodedBlockedGridVolumeWriter;


template<typename Voxel,VolumeType type>
struct VolumeTypeTraits;

#define Register_VolumeTypeTraits(type, name)\
template<typename Voxel>                     \
struct VolumeTypeTraits<Voxel,type>{         \
    using VolumeT = name<Voxel>;             \
    using ReaderT = name##Reader;            \
    using WriterT = name##Writer;            \
};

Register_VolumeTypeTraits(VolumeType::Grid_RAW,RawGridVolume)
Register_VolumeTypeTraits(VolumeType::Grid_SLICED,SlicedGridVolume)
Register_VolumeTypeTraits(VolumeType::Grid_BLOCKED,BlockedGridVolume)
Register_VolumeTypeTraits(VolumeType::Grid_ENCODED,EncodedGridVolume)
Register_VolumeTypeTraits(VolumeType::Grid_BLOCKED_ENCODED,EncodedBlockedGridVolume)

inline constexpr const char* VolumeTypeToStr(VolumeType type){
    switch (type) {
        case VolumeType::Grid_RAW : return "Grid_RAW";
        case VolumeType::Grid_SLICED : return "Grid_SLICED";
        case VolumeType::Grid_BLOCKED : return "Grid_BLOCKED";
        case VolumeType::Grid_ENCODED : return "Grid_ENCODED";
        case VolumeType::Grid_BLOCKED_ENCODED : return "Grid_BLOCKED_ENCODED";
        default : return "Grid_UNKNOWN";
    }
}

template<typename T>
class VolumeHelper{
public:
    template<typename F>
    T map(F&& f) const noexcept {
        return std::forward<T>(T::map(std::forward<F>(f)));
    }
    template<typename F>
    void map_inplace(F&& f) const noexcept{
        T::map_inplace(std::forward<F>(f));
    }


};

/**
 * @brief Memory model for volume.
 */
template<typename Voxel>
class VolumeInterface{
public:
    /**
     * @note Invalid params will not throw any exceptions and will just return 0.
     * @return The number of voxels which are read successfully.
     */
    virtual size_t ReadVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, Voxel *buf, size_t size) noexcept = 0;

    virtual size_t ReadVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, Voxel *buf, std::function<Voxel(const Voxel&)> mapper) noexcept = 0;

    virtual size_t WriteVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const Voxel *buf, size_t size) noexcept = 0;

    virtual size_t WriteVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, Voxel *buf, std::function<Voxel(const Voxel&)> mapper) noexcept = 0;

    virtual Voxel& operator()(int x, int y, int z) = 0;

    virtual const Voxel& operator()(int x, int y, int z) const = 0;

    virtual VolumeType GetVolumeType() const noexcept = 0;

    //...
};


using VolumeReadWriteFunc = std::function<void(const void* src, void* dst)>;

/**
 * @brief Common interface for reading volume.
 */
template<typename VolumeDescT>
class VolumeReaderInterface{
public:
    virtual size_t ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, void *buf, size_t size) noexcept = 0;
    virtual size_t ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadWriteFunc reader) noexcept = 0;
    virtual const VolumeDescT& GetVolumeDesc() const noexcept = 0;
};


/**
 * @brief Common interface for writing volume.
 */
template<typename VolumeDescT>
class VolumeWriterInterface{
public:
    virtual void SetVolumeDesc(const VolumeDescT&) noexcept = 0;
    virtual void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const void *buf, size_t size) noexcept = 0;
    virtual void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadWriteFunc writer) noexcept = 0;
};

struct VolumeDesc{

};

struct RawGridVolumeDesc : VolumeDesc{
    Extend3D extend;
    VoxelSpace space;
};

class RawGridVolumePrivate;
template<typename Voxel>
class RawGridVolume : public VolumeInterface<Voxel>, public VolumeHelper<RawGridVolume<Voxel>>{
public:
    using SelfT = RawGridVolume<Voxel>;

    RawGridVolume(const RawGridVolumeDesc &desc);

    ~RawGridVolume();

    const RawGridVolumeDesc &GetGridVolumeDesc() const;

public:
    size_t ReadVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, Voxel *buf, size_t size) noexcept override;

    size_t ReadVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, Voxel *buf, std::function<Voxel(const Voxel&)> mapper) noexcept override;

    size_t WriteVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const Voxel *buf, size_t size) noexcept override;

    size_t WriteVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, Voxel *buf, std::function<Voxel(const Voxel&)> mapper) noexcept override;

    Voxel& operator()(int x,int y,int z) override;

    const Voxel& operator()(int x,int y,int z) const override;
public:
    SelfT map() const noexcept;

    void map_inplace() noexcept;
public:
    Voxel* GetRawData() noexcept;

    const Voxel* GetRawData() const noexcept;
private:
    std::unique_ptr<RawGridVolumePrivate> _;
};

class RawGridVolumeReaderPrivate;
class RawGridVolumeReader : public VolumeReaderInterface<RawGridVolumeDesc>{
public:
    explicit RawGridVolumeReader(const std::string& filename);

    ~RawGridVolumeReader();

    size_t ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, void *buf, size_t size) noexcept override;

    size_t ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadWriteFunc reader) noexcept override;

    const RawGridVolumeDesc& GetVolumeDesc() const noexcept override;
private:
    std::unique_ptr<RawGridVolumeReaderPrivate> _;
};

class RawGridVolumeWriterPrivate;
class RawGridVolumeWriter : public VolumeWriterInterface<RawGridVolumeDesc>{
public:
    explicit RawGridVolumeWriter(const std::string& filename);

    ~RawGridVolumeWriter();

    void SetVolumeDesc(const RawGridVolumeDesc&) noexcept override;

    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const void *buf, size_t size) noexcept override;

    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadWriteFunc writer) noexcept override;
private:
    std::unique_ptr<RawGridVolumeWriterPrivate> _;
};



enum class SliceAxis : int{
    AXIS_X = 0,AXIS_Y = 1,AXIS_Z = 2
};
struct SlicedGridVolumeDesc : RawGridVolumeDesc{
    SliceAxis axis;
    std::function<std::string(uint32_t)> name_generator;
    std::string prefix;
    std::string postfix;
    int setw = 0;
};

class SlicedGridVolumePrivate;

template<typename Voxel>
class SlicedGridVolume :public RawGridVolume<Voxel>{
public:
    using VoxelT = Voxel;
    SlicedGridVolume(const SlicedGridVolumeDesc &desc, const std::string filename);

    ~SlicedGridVolume();

    const SlicedGridVolumeDesc &GetSlicedGridVolumeDesc() const;

    void ReadSlice(uint32_t z, Voxel *buf);

    size_t ReadVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, Voxel *buf) override;

    Voxel* GetRawSliceData() noexcept;
    const Voxel* GetRawSliceData() const noexcept;
    /**
     * @brief If set use cached, ReadVoxels will first try to read data from cache buffer and read entire slice if can, so it will cost more memory.
     */
    void SetUseCached(bool useCached);

    bool GetIfUseCached() const;
private:
    std::unique_ptr<SlicedGridVolumePrivate> _;
};

class SlicedGridVolumeReader : public VolumeReaderInterface<SlicedGridVolumeDesc>{
public:

};

struct BlockIndex {
    uint32_t x;
    uint32_t y;
    uint32_t z;
    bool operator==(const BlockIndex& blockIndex) const{
        return x == blockIndex.x && y == blockIndex.y && z == blockIndex.z;
    }
};

struct BlockedGridVolumeDesc : RawGridVolumeDesc{
    uint32_t block_length;
    uint32_t padding;
};

template <typename Voxel>
class BlockedGridVolume:public RawGridVolume<Voxel>{
public:
    BlockedGridVolume(const BlockedGridVolumeDesc&,const std::string&);

    using VoxelT = typename RawGridVolume<Voxel>::VoxelT;

    virtual void ReadBlockData(const BlockIndex&, VoxelT* buf);

    Voxel* GetRawBlockData(const BlockIndex&) noexcept;
    const Voxel* GetRawBlockData(const BlockIndex&) const noexcept;
};

class BlockedGridVolumeReader : public VolumeReaderInterface<BlockedGridVolumeDesc>{
public:

};

enum class GridVolumeDataCodec:int {
    GRID_VOLUME_CODEC_NONE = 0,
    GRID_VOLUME_CODEC_VIDEO = 1,
    GRID_VOLUME_CODEC_BYTES = 2
};

using Packet = std::vector<uint8_t>;
using Packets = std::vector<Packet>;
using EncodeWorker = std::function<bool(const void*,Packets&)>;
using DecodeWorker = std::function<bool(const Packets&,void*)>;

struct EncodedGridVolumeDesc : RawGridVolumeDesc{
    GridVolumeDataCodec codec;
    VoxelInfo voxel;
};

template <typename Voxel>
class EncodedGridVolume : public RawGridVolume<Voxel>{
public:

};

class EncodedGridVolumeReader : public VolumeReaderInterface<EncodedGridVolumeReader>{
public:

};

class EncodedGridVolumeWriter : public VolumeWriterInterface<EncodedGridVolumeWriter>{
public:

};

//todo virtual derived?
struct EncodedBlockedGridVolumeDesc : BlockedGridVolumeDesc{
    GridVolumeDataCodec codec;
    VoxelInfo voxel;
    char preserve[20];
};//64

//inline constexpr int EncodedBlockedGridVolumeDescSize = 64;
//static_assert(sizeof(EncodedBlockedGridVolumeDesc) == EncodedBlockedGridVolumeDescSize,"");

inline bool CheckValidation(const EncodedBlockedGridVolumeDesc& desc){
    if(desc.block_length == 0 || desc.padding == 0 || desc.block_length <= (desc.padding << 1)){
        return false;
    }
    if(desc.extend.width == 0 || desc.extend.height == 0 || desc.extend.depth == 0){
        return false;
    }
    return true;
}

template<typename Voxel>
class EncodedBlockedGridVolume: public BlockedGridVolume<Voxel>{
public:
    EncodedBlockedGridVolume(const EncodedBlockedGridVolumeDesc& desc, const std::string& filename);
};


class EncodedBlockedGridVolumeReaderPrivate;
class EncodedBlockedGridVolumeReader : public VolumeReaderInterface<EncodedBlockedGridVolumeDesc>{
public:
    explicit EncodedBlockedGridVolumeReader(const std::string &filename);

    ~EncodedBlockedGridVolumeReader();

    void Close();

    const EncodedBlockedGridVolumeDesc &GetEncodedBlockedGridVolumeDesc() const;

    void ReadBlockData(const BlockIndex &, void* buf, DecodeWorker worker = nullptr);

    void ReadRawBlockData(const BlockIndex &, Packet &);

    void ReadRawBlockData(const BlockIndex &, Packets &);
private:
    std::unique_ptr<EncodedBlockedGridVolumeReaderPrivate> _;
};

class EncodedBlockedGridVolumeWriterPrivate;
class EncodedBlockedGridVolumeWriter : public VolumeWriterInterface<EncodedGridVolumeWriter>{
public:
    EncodedBlockedGridVolumeWriter(const std::string &filename,const EncodedBlockedGridVolumeDesc& desc);

    ~EncodedBlockedGridVolumeWriter();

    const EncodedBlockedGridVolumeDesc &GetEncodedBlockedGridVolumeDesc() const;

    void Close();

    void WriteBlockData(const BlockIndex &, const void *buf, EncodeWorker worker = nullptr);

    void WriteRawBlockData(const BlockIndex &, const Packet &);

    void WriteRawBlockData(const BlockIndex &, const Packets &);
private:
    std::unique_ptr<EncodedBlockedGridVolumeWriterPrivate> _;
};

template<typename T>
class GridDataView;

template<typename T>
class SliceDataView {
public:
    SliceDataView(uint32_t sizeX, uint32_t sizeY, T *srcP = nullptr,std::function<T(int,int)> map = nullptr)
    :sizeX(sizeX),sizeY(sizeY),data(srcP),map(map)
    {}

    T At(int x,int y) const{
        if(map){
            return map(x,y);
        }
        return data[x + y * sizeX];
    }

    template<typename>
    friend class GridDataView;

    T* data;
    uint32_t sizeX,sizeY;
private:
    std::function<T(int x,int y)> map;
};

template<typename T>
class GridDataView {
public:
    GridDataView(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ,const T *srcP)
    :size_x(sizeX),size_y(sizeY),size_z(sizeZ),data(srcP)
    {
        assert(data && size_x && size_y && size_z);
    }

    SliceDataView<T> ViewSliceX(uint32_t x){
        std::function<size_t(int,int)> map = [x,this](int z,int y){
            return this->At(x,size_y - 1 - y,z);
        };
        return SliceDataView<T>(size_z,size_y,nullptr,map);
    }

    SliceDataView<T> ViewSliceY(uint32_t y){
        std::function<size_t(int,int)> map = [y,this](int x,int z){
            return this->At(x,y,size_z - 1 -z);
        };
        return SliceDataView<T>(size_x,size_z,nullptr,map);
    }

    SliceDataView<T> ViewSliceZ(uint32_t z){
        return SliceDataView<T>(size_x,size_y,data + (size_t)z * size_x * size_y);
    }

    const T& At(int x, int y, int z) const{
        if(x < 0 || x >= size_x || y < 0 || y >= size_y || z < 0 || z >= size_z){
            return T();
        }
        size_t idx = size_t(z) * size_x * size_y + y * size_x +x;
        return data[idx];
    }

    uint32_t size_x = 0,size_y = 0,size_z = 0;
    const T* data;
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
class VideoCodec<T,CodecDevice::CPU
        ,std::enable_if_t<VideoCodecVoxelV<T>>
        >{
public:
    VideoCodec() = delete;
    inline static int MaxThreads = 8;
    static bool Encode(const std::vector<SliceDataView<T>> &slices, Packet &packet,int maxThreads = MaxThreads);
    static bool Encode(const GridDataView<T>& volume,SliceAxis axis, Packet &packet,int maxThreads = MaxThreads);
    static bool Encode(const std::vector<SliceDataView<T>> &slices, Packets &packets,int maxThreads = MaxThreads);
    static bool Encode(const GridDataView<T>& volume,SliceAxis axis, Packets &packets,int maxThreads = MaxThreads);

    static bool Decode(const Packet &packet, T* buf,int maxThreads = MaxThreads);

    static bool Decode(const Packets &packets, T* buf,int maxThreads = MaxThreads);
};

bool EncodeBits(const Extend3D& shape,const void* buf,Packets& packets,int bitsPerSample,int nSamples,int maxThreads = 1);

bool DecodeBits(const Packets& packets,void* buf,int bitsPerSample,int nSamples,int maxThreads = 1);

template<typename T>
class VideoCodec<T,CodecDevice::GPU
        ,std::enable_if_t<VideoCodecVoxelV<T>>
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


struct VolumeFileDesc{
    union{
      RawGridVolumeDesc raw_desc;
      SlicedGridVolumeDesc sliced_desc;
      BlockedGridVolumeDesc blocked_desc;
      EncodedGridVolumeDesc encoded_desc;
      EncodedBlockedGridVolumeDesc encoded_blocked_desc;
    };
};

// volume describe data and meta data path
class VolumeFile{
public:
    VolumeFile(const std::string& filename);

    ~VolumeFile();

    VolumeType GetVolumeType() const noexcept;

    std::pair<VoxelType,VoxelFormat> GetVoxelInfo() const noexcept;

    const VolumeFileDesc& GetVolumeFileDesc() const noexcept;
};


template<typename Voxel, VolumeType type>
class VolumeIOWrapper{
public:
    using VolumeT = typename VolumeTypeTraits<Voxel,type>::VolumeT;

    static std::unique_ptr<VolumeT> LoadFromFile(const VolumeFile& volumeFile);

};

template<typename Voxel>
class RawGridVolumeIOWrapper : public VolumeIOWrapper<Voxel,VolumeType::Grid_RAW>{
public:
    using BaseT = VolumeIOWrapper<Voxel, VolumeType::Grid_RAW>;
    using VolumeT = typename BaseT::VolumeT;

    static std::unique_ptr<VolumeT> LoadFromMetaFile(const std::string& filename,const RawGridVolumeDesc& desc);

    static void SaveVolumeToMetaFile(const VolumeT& volume, const std::string& filename) noexcept;
};

template<typename Voxel>
class SlicedGridVolumeIOWrapper : public VolumeIOWrapper<Voxel, VolumeType::Grid_SLICED>{
public:
    using BaseT = VolumeIOWrapper<Voxel, VolumeType::Grid_SLICED>;
    using VolumeT = typename BaseT::VolumeT;

    static std::unique_ptr<VolumeT> LoadFromMetaFile(const SlicedGridVolumeDesc& desc);

    static void SaveVolumeToMetaFile(const VolumeT& volume, const std::string& filename) noexcept;
};

template<typename Voxel>
class EncodedBlockedGridVolumeIOWrapper : public VolumeIOWrapper<Voxel,VolumeType::Grid_BLOCKED_ENCODED>{
public:
    using BaseT = VolumeIOWrapper<Voxel, VolumeType::Grid_BLOCKED_ENCODED>;
    using VolumeT = typename BaseT::VolumeT;

    static std::unique_ptr<VolumeT> LoadFromMetaFile(const std::string& filename, const EncodedBlockedGridVolumeDesc& desc);

};


VOL_END

template<>
struct std::hash<vol::BlockIndex>{
    size_t operator()(const vol::BlockIndex& blockIndex) const{
        auto a = std::hash<uint32_t>()(blockIndex.x);
        auto b = std::hash<uint32_t>()(blockIndex.y);
        auto c = std::hash<uint32_t>()(blockIndex.z);
        return a ^ (b + 0x9e3779b97f4a7c15LL + (c << 6) + (c >> 2));
    }
};