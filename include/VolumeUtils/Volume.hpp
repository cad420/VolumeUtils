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
    static constexpr VoxelType type = VoxelType::uint8;
    static constexpr VoxelFormat format = VoxelFormat::R;
    static constexpr size_t size() { return 1; }
};
using VoxelRU8 = Voxel<VoxelType::uint8,VoxelFormat::R>;
static_assert(sizeof(VoxelRU8) == 1, "");

template<>
struct Voxel<VoxelType::uint16, VoxelFormat::R> {
    union {
        uint16_t x = 0;
        uint16_t r;
    };
    static constexpr VoxelType type = VoxelType::uint16;
    static constexpr VoxelFormat format = VoxelFormat::R;
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
    virtual const VolumeDescT& GetVolumeDesc() const noexcept = 0;
};

inline constexpr const char* InvalidVolumeName = "InvalidVolume";

struct VolumeDesc{
    // empty is not invalid
    std::string volume_name;
    // raw data file path
    std::string data_path;
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
    /**
     * @param filename descriptor json file path
     */
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
    explicit RawGridVolumeWriter(const std::string& filename, const RawGridVolumeDesc& desc);

    ~RawGridVolumeWriter();
public:
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
     * @note Internal implement for cache is simple, user can turn off cache and create cache by user self.
     */
    void SetUseCached(bool useCached);

    bool GetIfUseCached() const;

protected:
    std::unique_ptr<SlicedGridVolumeReaderPrivate> _;
};

class SlicedGridVolumeWriterPrivate;
class SlicedGridVolumeWriter : public VolumeWriterInterface<SlicedGridVolumeDesc>{
public:
    SlicedGridVolumeWriter(const std::string& filename, const SlicedGridVolumeDesc& desc);

    ~SlicedGridVolumeWriter();

public:

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
    int x;
    int y;
    int z;
    bool operator==(const BlockIndex& blockIndex) const{
        return x == blockIndex.x && y == blockIndex.y && z == blockIndex.z;
    }
};

struct BlockedGridVolumeDesc : RawGridVolumeDesc{
    uint32_t block_length;
    // physical stored block size = block_length + 2 * padding
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
    BlockedGridVolumeWriter(const std::string& filename, const BlockedGridVolumeDesc& desc);

    ~BlockedGridVolumeWriter();

public:
    const BlockedGridVolumeDesc& GetVolumeDesc() const noexcept override;

    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const void *buf, size_t size) noexcept override;

    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeWriteFunc writer) noexcept override;

public:
    void WriteBlockData(const BlockIndex& blockIndex, const void* buf, size_t size) noexcept;

    void WriteBlockData(const BlockIndex& blockIndex, VolumeWriteFunc writer) noexcept;

private:
    std::unique_ptr<BlockedGridVolumeWriterPrivate> _;
};

enum class GridVolumeCodec: int {
    GRID_VOLUME_CODEC_NONE = 0,
    GRID_VOLUME_CODEC_VIDEO = 1,
    GRID_VOLUME_CODEC_BITS = 2
};

using Packet = std::vector<uint8_t>;
using Packets = std::vector<Packet>;
using EncodeWorker = std::function<size_t(const void*, Packets&)>;
using DecodeWorker = std::function<size_t(const Packets&, void*)>;

struct EncodedGridVolumeDesc : RawGridVolumeDesc{
    GridVolumeCodec codec;
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
    EncodedGridVolumeWriter(const std::string& filename, const EncodedGridVolumeDesc& desc);

    ~EncodedGridVolumeWriter();

public:
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
    GridVolumeCodec codec;
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
    /**
     * @brief Read one block data into buf with size, it will use default(cpu) and suit codec for decoding.
     */
    size_t ReadBlockData(const BlockIndex& blockIndex, void* buf, size_t size) noexcept;

    /**
     * @brief This may be some slow but support more freedom.
     */
    size_t ReadBlockData(const BlockIndex& blockIndex, VolumeReadFunc reader) noexcept;

    /**
     * @param size should greater to equal to block bytes.
     * @note read data format is : [(packet_size)(packet_data)][(packet_size)(packet_data)]...
     * @return Exactly filled byte count.
     */
    size_t ReadEncodedBlockData(const BlockIndex& blockIndex, void* buf, size_t size) noexcept;

    /**
     * @return packets' bytes count
     */
    size_t ReadEncodedBlockData(const BlockIndex& blockIndex, Packets& packets) noexcept;

private:
    std::unique_ptr<EncodedBlockedGridVolumeReaderPrivate> _;
};

class EncodedBlockedGridVolumeWriterPrivate;
class EncodedBlockedGridVolumeWriter : public VolumeWriterInterface<EncodedBlockedGridVolumeDesc>{
public:
    /**
     * @param filename
     */
    EncodedBlockedGridVolumeWriter(const std::string& filename, const EncodedBlockedGridVolumeDesc& desc);

    ~EncodedBlockedGridVolumeWriter();

public:
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

/**
 * @brief like string_view.
 * @note only parse cpu ptr, or if you know what you are doing.
 */
template<typename T>
class SliceDataView {
public:
    SliceDataView()
    : SliceDataView(0, 0, nullptr, nullptr)
    {}

    SliceDataView(uint32_t sizeX, uint32_t sizeY,const T *srcP = nullptr,std::function<T(int,int)> map = nullptr)
    :sizeX(sizeX),sizeY(sizeY),data(srcP),map(map)
    {}

    const T& At(int x,int y) const{
        if(map){
            return map(x,y);
        }
        return data[x + y * sizeX];
    }

    template<typename>
    friend class GridDataView;

    const T* data;
    uint32_t sizeX,sizeY;
private:
    std::function<T(int x,int y)> map;
};

template<typename T>
class GridDataView {
public:
    GridDataView()
    : GridDataView(0,0,0,nullptr)
    {}

    GridDataView(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ,const T *srcP)
    :size_x(sizeX),size_y(sizeY),size_z(sizeZ),data(srcP)
    {
        assert(data && size_x && size_y && size_z);
    }

    SliceDataView<T> ViewSliceX(uint32_t x) const {
        std::function<const T&(int,int)> map = [x,this](int z,int y){
            return this->At(x,size_y - 1 - y,z);
        };
        return SliceDataView<T>(size_z,size_y,nullptr,map);
    }

    SliceDataView<T> ViewSliceY(uint32_t y) const {
        std::function<const T&(int,int)> map = [y,this](int x,int z){
            return this->At(x,y,size_z - 1 -z);
        };
        return SliceDataView<T>(size_x,size_z,nullptr,map);
    }

    SliceDataView<T> ViewSliceZ(uint32_t z) const {
        return SliceDataView<T>(size_x, size_y, data + (size_t)z * size_x * size_y, nullptr);
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

// one voxel type map to one specific codec method and format
Register_VoxelVideoCodec(VoxelRU8,CodecDevice::CPU)
Register_VoxelVideoCodec(VoxelRU8,CodecDevice::GPU)
Register_VoxelVideoCodec(VoxelRU16,CodecDevice::CPU)

class CVolumeCodecInterface{
public:
    ~CVolumeCodecInterface() = default;

    virtual GridVolumeCodec GetCodecType() const noexcept = 0;

    virtual CodecDevice GetCodecDevice() const noexcept = 0;
    /**
     * @note These infos may be not enough so we need derived this interface with template class with Voxel info.
     */
    virtual bool Encode(const Extend3D& extend, const void* buf, size_t size, Packets& packets) noexcept = 0;

    virtual bool Decode(const Extend3D &extend, const Packets &packets, void* buf, size_t size) noexcept = 0;

};


template<typename T>
class VolumeCodecInterface : public CVolumeCodecInterface{
public:
    ~VolumeCodecInterface() = default;

    virtual size_t Encode(const std::vector<SliceDataView<T>> &slices, void* buf, size_t size) = 0;

    virtual size_t Encode(const GridDataView<T>& volume,SliceAxis axis, void* buf, size_t size) = 0;

    virtual bool Encode(const std::vector<SliceDataView<T>> &slices, Packets &packets) = 0;

    virtual bool Encode(const GridDataView<T>& volume,SliceAxis axis, Packets &packets) = 0;

};

template<typename T>
class VolumeBitsCodecInterface : public VolumeCodecInterface<T>{
public:
    GridVolumeCodec GetCodecType() const noexcept override{
        return GridVolumeCodec::GRID_VOLUME_CODEC_BITS;
    }
};

template<typename T>
class VolumeVideoCodecInterface : public VolumeCodecInterface<T>{
public:
    GridVolumeCodec GetCodecType() const noexcept override{
        return GridVolumeCodec::GRID_VOLUME_CODEC_VIDEO;
    }
};

template<typename T,CodecDevice device,typename V = void>
class VolumeVideoCodec;

class CPUVolumeVideoCodecPrivate;
template<typename T>
class VolumeVideoCodec<T,CodecDevice::CPU,std::enable_if_t<VoxelVideoCodecV<T,CodecDevice::CPU>>>
        : public VolumeVideoCodecInterface<T>{
public:
    explicit VolumeVideoCodec(int threadCount = 1);

    CodecDevice GetCodecDevice() const noexcept override{
        return CodecDevice::CPU;
    }

    bool Encode(const Extend3D& extend, const void* buf, size_t size, Packets& packets) noexcept override;

    bool Decode(const Extend3D &extend, const Packets &packets, void* buf, size_t size) noexcept override;

public:
    size_t Encode(const std::vector<SliceDataView<T>> &slices, void* buf, size_t size) override;

    size_t Encode(const GridDataView<T>& volume,SliceAxis axis, void* buf, size_t size) override;

    bool Encode(const std::vector<SliceDataView<T>> &slices, Packets &packets) override;

    bool Encode(const GridDataView<T>& volume,SliceAxis axis, Packets &packets) override;

private:
    std::unique_ptr<CPUVolumeVideoCodecPrivate> _;
};

template<typename T>
using CPUVolumeVideoCodec = VolumeVideoCodec<T, CodecDevice::CPU, std::enable_if_t<VoxelVideoCodecV<T,CodecDevice::CPU>>>;


class GPUVolumeVideoCodecPrivate;
template<typename T>
class VolumeVideoCodec<T,CodecDevice::GPU,std::enable_if_t<VoxelVideoCodecV<T,CodecDevice::GPU>>>
        : public VolumeVideoCodecInterface<T>{
public:
    explicit VolumeVideoCodec(int GPUIndex = 0);
    //cuda context
    explicit VolumeVideoCodec(void* context);

    CodecDevice GetCodecDevice() const noexcept override{
        return CodecDevice::GPU;
    }

    /**
     * @param buf cpu or gpu ptr are both ok
     */
    bool Encode(const Extend3D& extend, const void* buf, size_t size, Packets& packets) noexcept override;

    /**
     * @param buf cpu or gpu ptr are both ok
     */
    bool Decode(const Extend3D &extend, const Packets &packets, void* buf, size_t size) noexcept override;

public:
    /**
     * @param buf only support cpu ptr now, maybe support device ptr next version...
     */
    size_t Encode(const std::vector<SliceDataView<T>> &slices, void* buf, size_t size) noexcept override;

    /**
     * @param buf only support cpu ptr now, maybe support device ptr next version...
     */
    size_t Encode(const GridDataView<T>& volume,SliceAxis axis, void* buf, size_t size) noexcept override;

    bool Encode(const std::vector<SliceDataView<T>> &slices, Packets &packets) noexcept override;

    bool Encode(const GridDataView<T>& volume,SliceAxis axis, Packets &packets) noexcept override;

private:
    std::unique_ptr<GPUVolumeVideoCodecPrivate> _;
};

template<typename T>
using GPUVolumeVideoCodec = VolumeVideoCodec<T, CodecDevice::GPU, std::enable_if_t<VoxelVideoCodecV<T,CodecDevice::GPU>>>;


struct VolumeFileDesc{
    //todo use std::variant
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
        auto a = std::hash<int>()(blockIndex.x);
        auto b = std::hash<int>()(blockIndex.y);
        auto c = std::hash<int>()(blockIndex.z);
        return a ^ (b + 0x9e3779b97f4a7c15LL + (c << 6) + (c >> 2));
    }
};


// imply template class VolumeVideoCodec
VOL_BEGIN

class VideoCodec{
public:
    virtual ~VideoCodec() = default;

    struct CodecParams{
        int frame_w = 0;
        int frame_h = 0;
        int frame_n = 0;
        int samplers_per_pixel = 0;
        int bits_per_sampler = 0;
        int threads_count = 1;
        bool encode = true;
        int device_index = 0;
        void* context = nullptr;
    };

    static std::unique_ptr<VideoCodec> Create(CodecDevice device);

    /**
     * @brief Clear status, must call after each volume encoded or decoded!!!
     */
    virtual bool ReSet(const CodecParams& params) = 0;

    /**
     * @brief encode one frame return put results in packets
     * @remark maybe change to a more flexible interface
     */
    virtual void EncodeFrameIntoPackets(const void* buf, size_t size, Packets& packets) = 0;

    virtual size_t DecodePacketIntoFrames(const Packet& packet, void* buf, size_t size) = 0;
};

class CPUVolumeVideoCodecPrivate{
public:
    std::unique_ptr<VideoCodec> video_codec;
    int thread_count = 0;
};

template<typename T>
CPUVolumeVideoCodec<T>::VolumeVideoCodec(int threadCount) {
    _->video_codec = VideoCodec::Create(CodecDevice::CPU);
    _->thread_count = threadCount;
}

template<typename T>
bool CPUVolumeVideoCodec<T>::Encode(const Extend3D &extend, const void *buf, size_t size, Packets &packets) noexcept {
    auto [w, h, d] = extend;
    VideoCodec::CodecParams params{
            (int)w,(int)h,(int)d,
            GetVoxelSampleCount(T::format),
            GetVoxelBits(T::type),
            _->thread_count
    };
    if(!_->video_codec->ReSet(params)) return false;
//    auto p = reinterpret_cast<const VoxelRU8 *>(buf);
    GridDataView<T> data_view(w, h, d, reinterpret_cast<const T*>(buf));
    auto voxel_size = GetVoxelSize(T::type, T::format);
    const size_t slice_size = w * d * voxel_size;
    // only encode buf with size
    d = size / slice_size;
    for(int z = 0; z < d; z++){
        Packets tmp_packets;
        _->video_codec->EncodeFrameIntoPackets(data_view.ViewSliceZ(z).data, slice_size, tmp_packets);
        packets.insert(packets.end(), tmp_packets.begin(), tmp_packets.end());
    }
    return true;
}

template<typename T>
bool CPUVolumeVideoCodec<T>::Decode(const Extend3D &extend, const Packets &packets, void *buf, size_t size) noexcept {
    if(!buf || !size) return false;

    auto voxel_size = GetVoxelSize(T::type, T::format);
    if(extend.size() * voxel_size > size){
        throw std::runtime_error("target decode buffer size is not enough large!");
    }
    // cpp20
    VideoCodec::CodecParams params{
        .threads_count = _->thread_count,
        .encode = false
    };
    if(!_->video_codec->ReSet(params)) return false;

    auto dst_ptr = reinterpret_cast<uint8_t*>(buf);
    size_t decode_size = 0;
    for(auto& packet : packets){
        auto ret = _->video_codec->DecodePacketIntoFrames(packet, dst_ptr + decode_size, size - decode_size);
        decode_size += ret;
    }

    return true;
}

template<typename T>
size_t CPUVolumeVideoCodec<T>::Encode(const std::vector<SliceDataView<T>> &slices, void *buf, size_t size) {
    if(!buf || !size) return 0;
    if(slices.empty()) return 0;

    Packets packets;
    auto ret = Encode(slices, packets);
    if(!ret) return 0;
    // packets to linear buf
    size_t packet_size = 0;
    auto dst_ptr = reinterpret_cast<uint8_t*>(buf);
    for(auto& packet : packets){
        if(packet_size >= size) break;
        size_t s = packet.size();
        std::memcpy(dst_ptr + packet_size, &s, sizeof(size_t));
        packet_size += sizeof(size_t);
        std::memcpy(dst_ptr + packet_size, packet.data(), packet.size());
        packet_size += packet.size();
    }

    return packet_size;
}

template<typename T>
size_t CPUVolumeVideoCodec<T>::Encode(const GridDataView<T> &volume, SliceAxis axis, void *buf, size_t size) {
    std::vector<SliceDataView<T>> slices;
    auto generate_slices = [p = &volume, &slices](auto f, uint32_t count){
        slices.resize(count);
        for(int i = 0; i < count; i++)
            slices.emplace_back(std::invoke(f,p,i));
    };
    if(axis == SliceAxis::AXIS_X)
        generate_slices(&GridDataView<T>::ViewSliceX, volume.size_x);
    else if(axis == SliceAxis::AXIS_Y)
        generate_slices(&GridDataView<T>::ViewSliceY, volume.size_y);
    else if(axis == SliceAxis::AXIS_Z)
        generate_slices(&GridDataView<T>::ViewSliceZ, volume.size_z);
    else
        return 0;
    return Encode(slices, buf, size);
}

template<typename T>
bool CPUVolumeVideoCodec<T>::Encode(const std::vector<SliceDataView<T>> &slices, Packets &packets) {
    // SliceDataView' storage maybe not linear because of map
    auto voxel_size = T::size();
    auto width = slices.front().sizeX;
    auto height = slices.front().sizeY;
    auto depth = slices.size();
    auto slice_size = voxel_size * width * height;
    VideoCodec::CodecParams params{
            (int)width,(int)height,(int)depth,
            GetVoxelSampleCount(T::format),
            GetVoxelBits(T::type),
            _->thread_count
    };
    if(!_->video_codec->ReSet(params)) return false;
    std::vector<uint8_t> buffer(slices.size() * slice_size, 0);
    size_t offset = 0;
    for(auto& slice : slices){
        std::memcpy(buffer.data() + offset, slice.data, slice_size);
        offset += slice_size;
    }
    Extend3D extend = {width, height, depth};
    auto ret = Encode(extend, buffer.data(), buffer.size(), packets);
    return ret > 0;
}

template<typename T>
bool CPUVolumeVideoCodec<T>::Encode(const GridDataView<T> &volume, SliceAxis axis, Packets &packets) {
    std::vector<SliceDataView<T>> slices;
    auto generate_slices = [p = &volume, &slices](auto f, uint32_t count){
        slices.resize(count);
        for(int i = 0; i < count; i++)
            slices.emplace_back(std::invoke(f,p,i));
    };
    if(axis == SliceAxis::AXIS_X)
        generate_slices(&GridDataView<T>::ViewSliceX, volume.size_x);
    else if(axis == SliceAxis::AXIS_Y)
        generate_slices(&GridDataView<T>::ViewSliceY, volume.size_y);
    else if(axis == SliceAxis::AXIS_Z)
        generate_slices(&GridDataView<T>::ViewSliceZ, volume.size_z);
    else
        return false;

    return Encode(slices, packets);
}


class GPUVolumeVideoCodecPrivate{
public:
    std::unique_ptr<VideoCodec> video_codec;
    int gpu_index = 0;
    void* context = nullptr;

};


template<typename T>
GPUVolumeVideoCodec<T>::VolumeVideoCodec(int GPUIndex){
    _ = std::make_unique<GPUVolumeVideoCodecPrivate>();
    _->gpu_index = GPUIndex;
    _->video_codec = VideoCodec::Create(CodecDevice::GPU);
}

template<typename T>
GPUVolumeVideoCodec<T>::VolumeVideoCodec(void *context) {
    if(!context){
        throw std::runtime_error("Invalid nullptr context");
    }
    _ = std::make_unique<GPUVolumeVideoCodecPrivate>();
    _->context = context;
    _->video_codec = VideoCodec::Create(CodecDevice::GPU);
}

template<typename T>
bool GPUVolumeVideoCodec<T>::Encode(const Extend3D &extend, const void *buf, size_t size, Packets &packets) noexcept {
    auto [w, h, d] = extend;
    VideoCodec::CodecParams params{
            (int)w, (int)h,(int)d,
            GetVoxelSampleCount(T::format),
            GetVoxelBits(T::type), 1, true,
            _->gpu_index,
            _->context
    };
    // same with CPU
    if(!_->video_codec->ReSet(params)) return false;
    GridDataView<T> data_view(w, h, d, buf);
    auto voxel_size = GetVoxelSize(T::type, T::format);
    const size_t slice_size = w * d * voxel_size;
    // only encode buf with size
    d = size / slice_size;
    for(int z = 0; z < d; z++){
        Packets tmp_packets;
        _->video_codec->EncodeFrameIntoPackets(data_view.ViewSliceZ(z).data, slice_size, tmp_packets);
        packets.insert(packets.end(), tmp_packets.begin(), tmp_packets.end());
    }
    return true;
}

template<typename T>
bool GPUVolumeVideoCodec<T>::Decode(const Extend3D &extend, const Packets &packets, void *buf, size_t size) noexcept {
    if(!buf || !size) return false;

    auto voxel_size = GetVoxelSize(T::type, T::format);
    if(extend.size() * voxel_size > size){
        throw std::runtime_error("target decode buffer size is not enough large!");
    }
    // cpp20
    VideoCodec::CodecParams params{
            .encode = false,
            .device_index = _->gpu_index,
            .context = _->context
    };
    if(!_->video_codec->ReSet(params)) return false;

    auto dst_ptr = reinterpret_cast<uint8_t*>(buf);
    size_t decode_size = 0;
    for(auto& packet : packets){
        auto ret = _->video_codec->DecodePacketIntoFrames(packet, dst_ptr + decode_size, size - decode_size);
        decode_size += ret;
    }

    return true;
}

template<typename T>
size_t GPUVolumeVideoCodec<T>::Encode(const std::vector<SliceDataView<T>> &slices, void *buf, size_t size) noexcept {
    if(slices.empty() || !buf || !size) return 0;
    Packets packets;
    auto ret = Encode(slices, packets);
    if(!ret) return 0;
    // buf must be cpu
    (void)(*(uint8_t*)buf);
    // packets to linear buf
    size_t packet_size = 0;
    auto dst_ptr = reinterpret_cast<uint8_t*>(buf);
    for(auto& packet : packets){
        if(packet_size >= size) break;
        size_t s = packet.size();
        std::memcpy(dst_ptr + packet_size, &s, sizeof(size_t));
        packet_size += sizeof(size_t);
        std::memcpy(dst_ptr + packet_size, packet.data(), packet.size());
        packet_size += packet.size();
    }

    return packet_size;
}

template<typename T>
size_t GPUVolumeVideoCodec<T>::Encode(const GridDataView<T> &volume, SliceAxis axis, void *buf, size_t size) noexcept {
    std::vector<SliceDataView<T>> slices;
    auto generate_slices = [p = &volume, &slices](auto f, uint32_t count){
        slices.resize(count);
        for(int i = 0; i < count; i++)
            slices.emplace_back(std::invoke(f,p,i));
    };
    if(axis == SliceAxis::AXIS_X)
        generate_slices(&GridDataView<T>::ViewSliceX, volume.size_x);
    else if(axis == SliceAxis::AXIS_Y)
        generate_slices(&GridDataView<T>::ViewSliceY, volume.size_y);
    else if(axis == SliceAxis::AXIS_Z)
        generate_slices(&GridDataView<T>::ViewSliceZ, volume.size_z);
    else
        return 0;
    return Encode(slices, buf, size);
}

template<typename T>
bool GPUVolumeVideoCodec<T>::Encode(const std::vector<SliceDataView<T>> &slices, Packets &packets) noexcept {
    auto voxel_size = T::size();
    auto width = slices.front().sizeX;
    auto height = slices.front().sizeY;
    auto depth = slices.size();
    auto slice_ele_count = (size_t)width * height;
    auto slice_size = voxel_size * width * height;
    VideoCodec::CodecParams params{
            .frame_w = width,.frame_h = height,.frame_n = depth,
            .samplers_per_pixel = GetVoxelSampleCount(T::format),
            .bits_per_sampler = GetVoxelBits(T::type),
            .device_index = _->gpu_index,
            .context = _->context
    };
    if(!_->video_codec->ReSet(params)) return false;
    auto slice_buffer = std::vector<uint8_t>(slice_size, 0);
    auto dst_ptr = slice_buffer.data();
    for(auto& slice : slices){
        size_t offset = 0;
        for(int i = 0; i < height; i++)
            for(int j = 0; j < width; j++)
                std::memcpy(dst_ptr + offset, &slice.At(j, i), voxel_size), offset += voxel_size;
        assert(offset == slice_size);
        Packets tmp_packets;
        _->video_codec->EncodeFrameIntoPackets(dst_ptr, slice_size, tmp_packets);
        packets.insert(packets.end(), tmp_packets.begin(), tmp_packets.end());
    }
    return true;
}

template<typename T>
bool GPUVolumeVideoCodec<T>::Encode(const GridDataView<T> &volume, SliceAxis axis, Packets &packets) noexcept {
    std::vector<SliceDataView<T>> slices;
    auto generate_slices = [p = &volume, &slices](auto f, uint32_t count){
        slices.resize(count);
        for(int i = 0; i < count; i++)
            slices.emplace_back(std::invoke(f,p,i));
    };
    if(axis == SliceAxis::AXIS_X)
        generate_slices(&GridDataView<T>::ViewSliceX, volume.size_x);
    else if(axis == SliceAxis::AXIS_Y)
        generate_slices(&GridDataView<T>::ViewSliceY, volume.size_y);
    else if(axis == SliceAxis::AXIS_Z)
        generate_slices(&GridDataView<T>::ViewSliceZ, volume.size_z);
    else
        return false;

    return Encode(slices, packets);
}

VOL_END

// imply template class VolumeBitsCodec
VOL_BEGIN

class BitsCodec{
public:
    struct BitsCodecParams{

    };
    static std::unique_ptr<BitsCodec> Create(CodecDevice device);

};


VOL_END