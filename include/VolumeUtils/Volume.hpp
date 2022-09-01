#pragma once

#define VOL_BEGIN namespace vol{
#define VOL_END }
// DEBUG
#ifndef NDEBUG
#define VOL_DEBUG
#define VOL_WHEN_DEBUG(op) do{ op; }while(false);
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
#include <stdexcept>
#include <ostream>
#include <sstream>
#include <iomanip>

VOL_BEGIN

#include "Volume.inl"

class VolumeFileOpenError : public std::exception{
public:
    VolumeFileOpenError(const std::string& errMsg) : std::exception(errMsg.c_str()){}
};

class VolumeFileContextError : public std::exception{
public:
    VolumeFileContextError(const std::string& errMsg) : std::exception(errMsg.c_str()){}
};

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

    float voxel() const{
        return x * y * z;
    }

    friend std::ostream& operator<<(std::ostream& os, const VoxelSpace& space){
        os << "( " << space.x << ", " << space.y << ", " << space.z << ")";
        return os;
    }
};

struct VoxelInfo{
    VoxelType type;
    VoxelFormat format;
};



inline constexpr size_t GetVoxelSize(const VoxelInfo& info){
    return GetVoxelSize(info.type,info.format);
}

bool CheckValidation(const VoxelInfo& info){
    return GetVoxelSize(info) != 0;
}

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

    friend std::ostream& operator<<(std::ostream& os, const Extend3D& extend) {
        os << "(" << extend.width << ", " << extend.height << ", " << extend.depth <<")";
        return os;
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
class VolumeInterface;
class VolumeDesc;
template <typename VolumeDescT>
class VolumeReaderInterface;
template <typename VolumeDescT>
class VolumeWriterInterface;
template<typename Voxel, VolumeType type>
class VolumeIOWrapper;

template <typename Voxel>
class RawGridVolume;
class RawGridVolumeDesc;
class RawGridVolumeReader;
class RawGridVolumeWriter;
template <typename Voxel>
class RawGridVolumeIOWrapper;

template <typename Voxel>
class EncodedGridVolume;
class EncodedGridVolumeDesc;
class EncodedGridVolumeReader;
class EncodedGridVolumeWriter;
template <typename Voxel>
class EncodedGridVolumeIOWrapper;

template<typename Voxel>
class SlicedGridVolume;
class SlicedGridVolumeDesc;
class SlicedGridVolumeReader;
class SlicedGridVolumeWriter;
template <typename Voxel>
class SlicedGridVolumeIOWrapper;

template <typename Voxel>
class BlockedGridVolume;
class BlockedGridVolumeDesc;
class BlockedGridVolumeReader;
class BlockedGridVolumeWriter;
template<typename Voxel>
class BlockedGridVolumeIOWrapper;

template<typename Voxel>
class EncodedBlockedGridVolume;
class EncodedBlockedGridVolumeDesc;
class EncodedBlockedGridVolumeReader;
class EncodedBlockedGridVolumeWriter;
template<typename Voxel>
class EncodedBlockedGridVolumeIOWrapper;

template<typename Voxel,VolumeType type>
struct VolumeTypeTraits;

#define Register_VolumeTypeTraits(type, name) \
template<typename Voxel>                      \
struct VolumeTypeTraits<Voxel,type>{          \
    using VolumeT    = name<Voxel>;           \
    using DescT      = name##Desc;            \
    using ReaderT    = name##Reader;          \
    using WriterT    = name##Writer;          \
    using IOWrapperT = name##IOWrapper<Voxel>;\
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
    T Map(F&& f) const noexcept {
        return std::forward<T>(T::Map(std::forward<F>(f)));
    }
    template<typename F>
    void MapInplace(F&& f) const noexcept{
        T::MapInplace(std::forward<F>(f));
    }
//    using DescT = typename VolumeTypeTraits<typename T::VoxelT,T::EVolumeType>::DescT;
    decltype(auto) GetVolumeDescT() const noexcept{
        return T::GetVolumeDescT();
    }

};


struct VolumeMemorySettings{
public:
    inline static size_t MaxMemoryUsageBytes = 16ull << 30;
    inline static size_t MaxSlicedGridMemoryUsageBytes = 16ull << 30;
};

class CVolumeInterface{
public:

};

/**
 * @brief Memory model for volume.
 */
template<typename Voxel>
class VolumeInterface : public CVolumeInterface{
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


using VolumeReadFunc = std::function<size_t(int dx, int dy, int dz, const void* src, size_t ele_size)>;
using VolumeWriteFunc = std::function<void(int dx, int dy, int dz, void* dst, size_t ele_size)>;

class CVolumeReaderInterface{
public:
    /**
     * @param size memory buffer length for buf, maybe less than bytes count for the read region or more than are both ok.
     */
    virtual size_t ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, void *buf, size_t size) noexcept = 0;
    virtual size_t ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadFunc reader) noexcept = 0;
};

/**
 * @brief Common interface for reading volume.
 */
template<typename VolumeDescT>
class VolumeReaderInterface : public CVolumeReaderInterface{
public:
    virtual const VolumeDescT& GetVolumeDesc() const noexcept = 0;
};


class CVolumeWriterInterface{
public:
    virtual void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const void *buf, size_t size) noexcept = 0;
    virtual void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeWriteFunc writer) noexcept = 0;
};

/**
 * @brief Common interface for writing volume.
 */
template<typename VolumeDescT>
class VolumeWriterInterface : public CVolumeWriterInterface{
public:
    virtual void SetVolumeDesc(const VolumeDescT&) noexcept = 0;

    virtual const VolumeDescT& GetVolumeDesc() const noexcept = 0;
};

inline constexpr const char* InvalidVolumeName = "InvalidVolume";

struct VolumeDesc{
    // empty is not invalid
    std::string volume_name;
    VoxelInfo voxel_info;
};

inline bool CheckValidation(const VolumeDesc& desc){
    return desc.volume_name != InvalidVolumeName
    && CheckValidation(desc.voxel_info);
}

struct RawGridVolumeDesc : VolumeDesc{
    Extend3D extend;
    VoxelSpace space;
};

inline bool CheckValidation(const RawGridVolumeDesc& desc){
    if(!CheckValidation(static_cast<const VolumeDesc&>(desc))) return false;
    return desc.extend.size() > 0 && desc.space.voxel() > 0.f;
}

/**
 * @note Create by constructor will just create a memory model, will not associate with file.
 */
class RawGridVolumePrivate;
template<typename Voxel>
class RawGridVolume : public VolumeInterface<Voxel>, public VolumeHelper<RawGridVolume<Voxel>>{
public:
    using VoxelT = Voxel;
    using SelfT = RawGridVolume<Voxel>;
    static constexpr VolumeType EVolumeType = VolumeType::Grid_RAW;

    RawGridVolume(const RawGridVolumeDesc &desc);

    ~RawGridVolume();

public:
    size_t ReadVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, Voxel *buf, size_t size) noexcept override;

    size_t ReadVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, Voxel *buf, std::function<Voxel(const Voxel&)> mapper) noexcept override;

    size_t WriteVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const Voxel *buf, size_t size) noexcept override;

    size_t WriteVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, Voxel *buf, std::function<Voxel(const Voxel&)> mapper) noexcept override;

    Voxel& operator()(int x,int y,int z) override;

    const Voxel& operator()(int x,int y,int z) const override;

    VolumeType GetVolumeType() const noexcept override;
public:
    // help functions
    template<typename F>
    SelfT Map(F&& f) const noexcept;

    template<typename F>
    void MapInplace(F&& f) noexcept;

    const RawGridVolumeDesc &GetVolumeDesc() const;
public:
    Voxel* GetRawDataPtr() noexcept;

    const Voxel* GetRawDataPtr() const noexcept;

protected:
    template<typename, VolumeType>
    friend class VolumeIOWrapper;
    template<typename>
    friend class RawGridVolumeIOWrapper;

    std::unique_ptr<RawGridVolumePrivate> _;
};

class RawGridVolumeReaderPrivate;
class RawGridVolumeReader : public VolumeReaderInterface<RawGridVolumeDesc>{
public:
    explicit RawGridVolumeReader(const std::string& filename);

    ~RawGridVolumeReader();
public:
    size_t ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, void *buf, size_t size) noexcept override;

    size_t ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadFunc reader) noexcept override;

    const RawGridVolumeDesc& GetVolumeDesc() const noexcept override;
private:
    std::unique_ptr<RawGridVolumeReaderPrivate> _;
};

class RawGridVolumeWriterPrivate;
class RawGridVolumeWriter : public VolumeWriterInterface<RawGridVolumeDesc>{
public:
    explicit RawGridVolumeWriter(const std::string& filename);

    ~RawGridVolumeWriter();
public:
    void SetVolumeDesc(const RawGridVolumeDesc&) noexcept override;

    const RawGridVolumeDesc& GetVolumeDesc() const noexcept override;

    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const void *buf, size_t size) noexcept override;

    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeWriteFunc writer) noexcept override;
private:
    std::unique_ptr<RawGridVolumeWriterPrivate> _;
};


enum class SliceAxis : int {
    AXIS_X = 0,AXIS_Y = 1,AXIS_Z = 2
};

struct SlicedGridVolumeDesc : RawGridVolumeDesc{
    SliceAxis axis;
    std::function<std::string(uint32_t)> name_generator;
    std::string prefix;
    std::string postfix;
    int setw = 0;

    void Generate() noexcept{
        name_generator = [this](uint32_t idx){
            std::stringstream ss;
            ss << prefix << std::setw(setw) << std::to_string(idx) << postfix;
            return ss.str();
        };
    }
};

bool CheckValidation(SliceAxis axis){
    return axis == SliceAxis::AXIS_X
    || axis == SliceAxis::AXIS_Y
    || axis == SliceAxis::AXIS_Z;
}

bool CheckValidation(const SlicedGridVolumeDesc& desc){
    if(!CheckValidation(static_cast<const RawGridVolumeDesc&>(desc))) return false;
    return CheckValidation(desc.axis) && desc.setw >= 0;
}

class SlicedGridVolumePrivate;
template<typename Voxel>
class SlicedGridVolume :public RawGridVolume<Voxel>{
public:
    size_t ReadSlice(SliceAxis axis, int sliceIndex, void* buf, size_t size) noexcept;
    size_t ReadSlice(int srcX, int srcY, int dstX, int dstY, SliceAxis axis, int sliceIndex, void* buf, size_t size) noexcept;
    size_t WriteSlice(SliceAxis axis, int sliceIndex, const void* buf, size_t size) noexcept;
    size_t WriteSlice(int srcX, int srcY, int dstX, int dstY, SliceAxis axis, int sliceIndex, const void* buf, size_t size) noexcept;

protected:
    std::unique_ptr<SlicedGridVolumePrivate> _;
};

using SliceReadFunc = std::function<size_t(int dx, int dy, const void* src, size_t ele_size)>;
using SliceWriteFunc = std::function<void(int dx, int dy, void* dst, size_t ele_size)>;
class SlicedGridVolumeReaderPrivate;
class SlicedGridVolumeReader : public VolumeReaderInterface<SlicedGridVolumeDesc>{
public:
    SlicedGridVolumeReader(const std::string& filename) noexcept(false);

    ~SlicedGridVolumeReader();

public:

    size_t ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, void *buf, size_t size) noexcept override;

    size_t ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadFunc reader) noexcept override;

    const SlicedGridVolumeDesc& GetVolumeDesc() const noexcept override;
public:
    /**
     * @brief Can only read axis return by GetVolumeDesc, use this if want to read more efficient.
     * Read a region data from slice, the region only require valid which means src < dst but is ok for src < 0 || dst > w/h.
     * Read data will linear fill the buf.
     * @param buf is the address of a linear buffer.
     * @param size buffer length for buf.
     * @return bytes count filled into the buf.
     */
    size_t ReadSliceData(int sliceIndex, int srcX, int srcY, int dstX, int dstY, void* buf, size_t size) noexcept;

    /**
     * @brief This method is mainly to read slice data store/map to a non-linear buffer.
     */
    size_t ReadSliceData(int sliceIndex, int srcX, int srcY, int dstX, int dstY, SliceReadFunc reader) noexcept;

    size_t ReadSliceData(int sliceIndex, void* buf, size_t size) noexcept;

    size_t ReadSliceData(int sliceIndex, SliceReadFunc reader) noexcept;
    /**
     * @brief If set use cached, reader will first try to read data from cache buffer and read entire slice if can,
     * so it will cost more memory if random access but will be more efficient for read by sequence for huge volume data.
     */
    void SetUseCached(bool useCached);

    bool GetIfUseCached() const;

protected:
    std::unique_ptr<SlicedGridVolumeReaderPrivate> _;
};

class SlicedGridVolumeWriterPrivate;
class SlicedGridVolumeWriter : public VolumeWriterInterface<SlicedGridVolumeDesc>{
public:
    SlicedGridVolumeWriter(const std::string& filename);

    ~SlicedGridVolumeWriter();

public:
    void SetVolumeDesc(const SlicedGridVolumeDesc&) noexcept override;

    const SlicedGridVolumeDesc& GetVolumeDesc() const noexcept override;

    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const void *buf, size_t size) noexcept override;

    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeWriteFunc writer) noexcept override;

public:
    void WriteSliceData(int sliceIndex, const void* buf, size_t size) noexcept;

    void WriteSliceData(int sliceIndex, SliceWriteFunc writer) noexcept;

    void WriteSliceData(int sliceIndex, int srcX, int srcY, int dstX, int dstY, const void* buf, size_t size) noexcept;

    void WriteSliceData(int sliceIndex, int srcX, int srcY, int dstX, int dstY, SliceWriteFunc writer) noexcept;

    void Flush() noexcept;

protected:
    std::unique_ptr<SlicedGridVolumeWriterPrivate> _;
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
    using VoxelT = typename RawGridVolume<Voxel>::VoxelT;

    BlockedGridVolume(const BlockedGridVolumeDesc&,const std::string&);

    virtual void ReadBlockData(const BlockIndex&, VoxelT* buf);

    Voxel* GetRawBlockData(const BlockIndex&) noexcept;
    const Voxel* GetRawBlockData(const BlockIndex&) const noexcept;
};

class BlockedGridVolumeReaderPrivate;
class BlockedGridVolumeReader : public VolumeReaderInterface<BlockedGridVolumeDesc>{
public:
    size_t ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, void *buf, size_t size) noexcept override;
    size_t ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadFunc reader) noexcept override;
    const BlockedGridVolumeDesc& GetVolumeDesc() const noexcept override;
public:
    size_t ReadBlockData(const BlockIndex& blockIndex, void* buf, size_t size) noexcept;
    size_t ReadBlockData(const BlockIndex& blockIndex, VolumeReadFunc reader) noexcept;

private:
    std::unique_ptr<BlockedGridVolumeReaderPrivate> _;
};


class BlockedGridVolumeWriterPrivate;
class BlockedGridVolumeWriter : public VolumeWriterInterface<BlockedGridVolumeDesc>{
public:
    void SetVolumeDesc(const BlockedGridVolumeDesc&) noexcept override;
    const BlockedGridVolumeDesc& GetVolumeDesc() const noexcept override;
    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const void *buf, size_t size) noexcept override;
    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeWriteFunc writer) noexcept override;
public:
    void WriteBlockData(const BlockIndex& blockIndex, const void* buf, size_t size) noexcept;
    void WriteBlockData(const BlockIndex& blockIndex, VolumeWriteFunc writer) noexcept;
private:
    std::unique_ptr<BlockedGridVolumeWriterPrivate> _;
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

class EncodedGridVolumeReaderPrivate;
class EncodedGridVolumeReader : public VolumeReaderInterface<EncodedGridVolumeDesc>{
public:
    // select cpu or gpu...
public:
    size_t ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, void *buf, size_t size) noexcept override;
    size_t ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadFunc reader) noexcept override;
    const EncodedGridVolumeDesc& GetVolumeDesc() const noexcept override;
public:
    size_t ReadEncodedData(void* buf, size_t size) noexcept;
    size_t ReadEncodedData(Packets& packets) noexcept;
private:
    std::unique_ptr<EncodedGridVolumeReaderPrivate> _;
};

class EncodedGridVolumeWriterPrivate;
class EncodedGridVolumeWriter : public VolumeWriterInterface<EncodedGridVolumeDesc>{
public:
    void SetVolumeDesc(const EncodedGridVolumeDesc&) noexcept override;
    const EncodedGridVolumeDesc& GetVolumeDesc() const noexcept override;
    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const void *buf, size_t size) noexcept override;
    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeWriteFunc writer) noexcept override;
public:
    void WriteEncodedData(const void* buf, size_t size) noexcept;
    void WriteEncodedData(const Packets& packets) noexcept;
private:
    std::unique_ptr<EncodedGridVolumeWriterPrivate> _;
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
    EncodedBlockedGridVolumeReader(const std::string& filename);

    ~EncodedBlockedGridVolumeReader();

public:
    size_t ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, void *buf, size_t size) noexcept override;

    size_t ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadFunc reader) noexcept override;

    const EncodedBlockedGridVolumeDesc& GetVolumeDesc() const noexcept override;

public:
    size_t ReadBlockData(const BlockIndex& blockIndex, void* buf, size_t size) noexcept;

    size_t ReadBlockData(const BlockIndex& blockIndex, VolumeReadFunc reader) noexcept;

    size_t ReadEncodedBlockData(const BlockIndex& blockIndex, void* buf, size_t size) noexcept;

    size_t ReadEncodedBlockData(const BlockIndex& blockIndex, Packets& packets) noexcept;

private:
    std::unique_ptr<EncodedBlockedGridVolumeReaderPrivate> _;
};

class EncodedBlockedGridVolumeWriterPrivate;
class EncodedBlockedGridVolumeWriter : public VolumeWriterInterface<EncodedBlockedGridVolumeDesc>{
public:
    EncodedBlockedGridVolumeWriter(const std::string& filename);

    ~EncodedBlockedGridVolumeWriter();

public:
    void SetVolumeDesc(const EncodedBlockedGridVolumeDesc&) noexcept override;

    const EncodedBlockedGridVolumeDesc& GetVolumeDesc() const noexcept override;

    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const void *buf, size_t size) noexcept override;

    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeWriteFunc writer) noexcept override;

public:
    void WriteBlockData(const BlockIndex& blockIndex, const void* buf, size_t size) noexcept;

    void WriteBlockData(const BlockIndex& blockIndex, VolumeWriteFunc writer) noexcept;

    void WriteEncodedBlockData(const BlockIndex& blockIndex, const void* buf, size_t size) noexcept;

    void WriteEncodedBlockData(const BlockIndex& blockIndex, const Packets& packets) noexcept;

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
template<typename Voxel,CodecDevice device>
struct VoxelVideoCodec{
    static constexpr bool Value = false;
};
#define Register_VoxelVideoCodec(VoxelT,device) \
template<>\
struct VoxelVideoCodec<VoxelT,device>{\
    static constexpr bool Value = true;        \
};


template<typename T, CodecDevice device>
inline constexpr bool VoxelVideoCodecV = VoxelVideoCodec<T,device>::Value;

Register_VoxelVideoCodec(VoxelRU8,CodecDevice::CPU)
Register_VoxelVideoCodec(VoxelRU8,CodecDevice::GPU)
Register_VoxelVideoCodec(VoxelRU16,CodecDevice::CPU)

class CVolumeVideoCodecInterface{
public:
    virtual bool Encode(const void* buf, size_t size, Packets& packets) noexcept = 0;
    virtual bool Decode(const Packets &packets, void* buf, size_t size) noexcept = 0;
};

template<typename T>
class VolumeVideoCodecInterface : public CVolumeVideoCodecInterface{
public:
    virtual bool Encode(const std::vector<SliceDataView<T>> &slices, void* buf, size_t size) = 0;
    virtual bool Encode(const GridDataView<T>& volume,SliceAxis axis, void* buf, size_t size) = 0;
    virtual bool Encode(const std::vector<SliceDataView<T>> &slices, Packets &packets) = 0;
    virtual bool Encode(const GridDataView<T>& volume,SliceAxis axis, Packets &packets) = 0;
};

template<typename T,CodecDevice device,typename V = void>
class VolumeVideoCodec;

template<typename T>
class VolumeVideoCodec<T,CodecDevice::CPU,std::enable_if_t<VoxelVideoCodecV<T,CodecDevice::CPU>>>
        : public VolumeVideoCodecInterface<T>{
public:
    explicit VolumeVideoCodec(int threadCount);

    bool Encode(const void* buf, size_t size, Packets& packets) noexcept override;
    bool Decode(const Packets &packets, void* buf, size_t size) noexcept override;
public:
    bool Encode(const std::vector<SliceDataView<T>> &slices, void* buf, size_t size) override;
    bool Encode(const GridDataView<T>& volume,SliceAxis axis, void* buf, size_t size) override;
    bool Encode(const std::vector<SliceDataView<T>> &slices, Packets &packets) override;
    bool Encode(const GridDataView<T>& volume,SliceAxis axis, Packets &packets) override;
};

template<typename T>
class VolumeVideoCodec<T,CodecDevice::GPU,std::enable_if_t<VoxelVideoCodecV<T,CodecDevice::GPU>>>
        : public VolumeVideoCodecInterface<T>{
public:
    explicit VolumeVideoCodec(int GPUIndex);
    //cuda context
    explicit VolumeVideoCodec(void* context);

    bool Encode(const void* buf, size_t size, Packets& packets) noexcept override;
    bool Decode(const Packets &packets, void* buf, size_t size) noexcept override;
public:
    bool Encode(const std::vector<SliceDataView<T>> &slices, void* buf, size_t size) noexcept override;
    bool Encode(const GridDataView<T>& volume,SliceAxis axis, void* buf, size_t size) noexcept override;
    bool Encode(const std::vector<SliceDataView<T>> &slices, Packets &packets) noexcept override;
    bool Encode(const GridDataView<T>& volume,SliceAxis axis, Packets &packets) noexcept override;
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

    //get tf...
};

/**
 * @note Volume model created by *IOWrapper will be associated with file.
 */
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
class BlockedGridVolumeIOWrapper : public VolumeIOWrapper<Voxel,VolumeType::Grid_BLOCKED>{
public:
    using BaseT = VolumeIOWrapper<Voxel, VolumeType::Grid_SLICED>;
    using VolumeT = typename BaseT::VolumeT;

    static std::unique_ptr<VolumeT> LoadFromMetaFile();

};


template<typename Voxel>
class EncodedGridVolumeIOWrapper : public VolumeIOWrapper<Voxel,VolumeType::Grid_ENCODED>{
public:
    using BaseT = VolumeIOWrapper<Voxel, VolumeType::Grid_BLOCKED_ENCODED>;
    using VolumeT = typename BaseT::VolumeT;

    static std::unique_ptr<VolumeT> LoadFromMetaFile();

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