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

// OS
#if defined(_WIN32)
#define VOL_OS_WIN32
#elif defined(__linux)
#define VOL_OS_LINUX
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

#define NOT_IMPL { std::cerr << "ERROR : Not Imply!!!" << std::endl; assert(false); }

#include <iostream>
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <cassert>
#include <stdexcept>
#include <ostream>
#include <sstream>
#include <iomanip>
#include <variant>

VOL_BEGIN

#include "Volume.inl"

class VolumeFileOpenError : public std::runtime_error{
public:
    VolumeFileOpenError(const std::string& errMsg) : std::runtime_error(errMsg.c_str()){}
};

class VolumeFileContextError : public std::runtime_error{
public:
    VolumeFileContextError(const std::string& errMsg) : std::runtime_error(errMsg.c_str()){}
};

class VolumeFileIOError : public std::runtime_error{
public:
    VolumeFileIOError(const std::string& errMsg) : std::runtime_error(errMsg.c_str()){}
};

class VolumeCodecError : public std::runtime_error{
public:
    VolumeCodecError(const std::string& errMgs) : std::runtime_error(errMgs.c_str()) {}
};

enum class VoxelType : uint32_t{
    unknown = 0, uint8, uint16, float32
};

enum class VoxelFormat : uint32_t{
    NONE = 0, R = 1, RG = 2, RGB = 3, RGBA = 4
};

inline constexpr bool IsVoxelTypeInteger(VoxelType type) noexcept {
    switch (type) {
        case VoxelType::uint8:
        case VoxelType::uint16: return true;
        case VoxelType::float32: return false;
        case VoxelType::unknown:
        default: return true;
    }
}

inline constexpr int GetVoxelSampleCount(VoxelFormat format) noexcept {
    switch (format) {
        case VoxelFormat::R: return 1;
        case VoxelFormat::RG: return 2;
        case VoxelFormat::RGB: return 3;
        case VoxelFormat::RGBA: return 4;
        default: return 0;
    }
}

inline constexpr int GetVoxelBits(VoxelType type) noexcept {
    switch (type) {
        case VoxelType::uint8: return 8;
        case VoxelType::uint16: return 16;
        case VoxelType::float32: return 32;
        default: return 0;
    }
}

inline constexpr size_t GetVoxelSize(VoxelType type, VoxelFormat format) noexcept {
    return GetVoxelBits(type) * GetVoxelSampleCount(format) >> 3;
}

struct VoxelSpace {
    float x = 1.f;
    float y = 1.f;
    float z = 1.f;

    float voxel() const noexcept{
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

inline constexpr size_t GetVoxelSize(const VoxelInfo& info) noexcept {
    return GetVoxelSize(info.type, info.format);
}

inline bool CheckValidation(const VoxelInfo& info) noexcept {
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
    using VoxelDataType = uint8_t;
    static constexpr VoxelType type = VoxelType::uint8;
    static constexpr VoxelFormat format = VoxelFormat::R;
    static constexpr size_t size() noexcept { return 1; }
};
using VoxelRU8 = Voxel<VoxelType::uint8, VoxelFormat::R>;
static_assert(sizeof(VoxelRU8) == 1, "");

template<>
struct Voxel<VoxelType::uint16, VoxelFormat::R> {
    union {
        uint16_t x = 0;
        uint16_t r;
    };
    using VoxelDataType = uint16_t;
    static constexpr VoxelType type = VoxelType::uint16;
    static constexpr VoxelFormat format = VoxelFormat::R;
    static constexpr size_t size() noexcept { return 2; }
};
using VoxelRU16 = Voxel<VoxelType::uint16, VoxelFormat::R>;
static_assert(sizeof(VoxelRU16) == 2);

template<>
struct Voxel<VoxelType::float32, VoxelFormat::R> {
    union {
        float x = 0.f;
        float r;
    };
    using VoxelDataType = float;
    static constexpr VoxelType type = VoxelType::float32;
    static constexpr VoxelFormat format = VoxelFormat::R;
    static constexpr size_t size() noexcept { return 4; }
};
using VoxelRF32 = Voxel<VoxelType::float32, VoxelFormat::R>;
static_assert(sizeof(VoxelRF32) == 4);

struct Extend3D {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 0;

    size_t size() const noexcept {
        return static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(depth);
    }

    friend std::ostream& operator<<(std::ostream& os, const Extend3D& extend) {
        os << "(" << extend.width << ", " << extend.height << ", " << extend.depth <<")";
        return os;
    }
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

inline constexpr const char* VolumeTypeToStr(VolumeType type) noexcept {
    switch (type) {
        case VolumeType::Grid_RAW :             return "Grid_RAW";
        case VolumeType::Grid_SLICED :          return "Grid_SLICED";
        case VolumeType::Grid_BLOCKED :         return "Grid_BLOCKED";
        case VolumeType::Grid_ENCODED :         return "Grid_ENCODED";
        case VolumeType::Grid_BLOCKED_ENCODED : return "Grid_BLOCKED_ENCODED";
        default :                               return "Grid_UNKNOWN";
    }
}

//TODO
struct VolumeMemorySettings{
public:
    inline static size_t MaxMemoryUsageBytes = 16ull << 30;
    inline static size_t MaxSlicedGridMemoryUsageBytes = 16ull << 30;
};

class CVolumeInterface{
public:
    virtual ~CVolumeInterface() = default;
};

template<typename Voxel>
using TVolumeReadFunc = std::function<void(int dx, int dy, int dz, const Voxel& dst)>;
template<typename Voxel>
using TVolumeWriteFunc = std::function<void(int dx, int dy, int dz, Voxel& dst)>;

template<typename Voxel>
class VolumeInterface : public CVolumeInterface{
public:
    ~VolumeInterface() override = default;
    /**
     * @param buf ptr to buffer which will be filled
     * @note dst must > src, and read region is [src, dst)
     * @note throw on error
     */
    virtual void ReadVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, Voxel *buf)                      = 0;

    virtual void ReadVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, TVolumeReadFunc<Voxel> reader)   = 0;

    virtual void WriteVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const Voxel *buf)               = 0;

    virtual void WriteVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, TVolumeWriteFunc<Voxel> writer) = 0;

    virtual Voxel& operator()(int x, int y, int z)             = 0;

    virtual const Voxel& operator()(int x, int y, int z) const = 0;

    virtual VolumeType GetVolumeType() const noexcept = 0;

};


using VolumeReadFunc  = std::function<void(int dx, int dy, int dz, const void* src, size_t ele_size)>;
using VolumeWriteFunc = std::function<void(int dx, int dy, int dz, void* dst, size_t ele_size)>;

class CVolumeReaderInterface{
public:
    virtual ~CVolumeReaderInterface() = default;
    /**
     * @note [src, dst)
     */
    virtual void ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, void *buf)             = 0;

    virtual void ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadFunc reader) = 0;
};

/**
 * @brief Common interface for reading volume.
 */
template<typename VolumeDescT>
class VolumeReaderInterface : public CVolumeReaderInterface{
public:
    ~VolumeReaderInterface() override = default;

    virtual VolumeDescT GetVolumeDesc() const noexcept = 0;
};

class CVolumeWriterInterface{
public:
    virtual ~CVolumeWriterInterface() = default;

    virtual void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const void *buf)        = 0;

    virtual void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeWriteFunc writer) = 0;
};

/**
 * @brief Common interface for writing volume.
 */
template<typename VolumeDescT>
class VolumeWriterInterface : public CVolumeWriterInterface{
public:
    ~VolumeWriterInterface() override = default;

    virtual VolumeDescT GetVolumeDesc() const noexcept = 0;
};

inline constexpr const char* InvalidVolumeName = "INVALID_VOLUME";

struct VolumeDesc{
    std::string volume_name = "default";
    std::string data_path;
    VoxelInfo voxel_info;
};

inline bool CheckValidation(const VolumeDesc& desc) noexcept {
    return desc.volume_name != InvalidVolumeName
    && CheckValidation(desc.voxel_info);
}

struct RawGridVolumeDesc : VolumeDesc{
    Extend3D extend;
    VoxelSpace space;
};

inline bool CheckValidation(const RawGridVolumeDesc& desc) noexcept {
    if(!CheckValidation(static_cast<const VolumeDesc&>(desc))) return false;
    return desc.extend.size() > 0 && desc.space.voxel() > 0.f;
}

/**
 * @note Create by constructor will just create a memory model, will not associate with file.
 */
class RawGridVolumePrivate;
template<typename Voxel>
class RawGridVolume : public VolumeInterface<Voxel>{
public:
    using VoxelT = Voxel;
    using SelfT = RawGridVolume<Voxel>;
    static constexpr VolumeType EVolumeType = VolumeType::Grid_RAW;

    explicit RawGridVolume(const RawGridVolumeDesc &desc);

    ~RawGridVolume() override;

    RawGridVolumeDesc GetVolumeDesc() const noexcept;

public:
    /**
     * @param buf ptr to buffer which will be filled, must large than read region
     * @note [src, dst)
     */
    void ReadVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, Voxel *buf)                      override;

    void ReadVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, TVolumeReadFunc<Voxel> reader)   override;

    void WriteVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const Voxel *buf)               override;

    void WriteVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, TVolumeWriteFunc<Voxel> writer) override;

    Voxel& operator()(int x,int y,int z) override;

    const Voxel& operator()(int x,int y,int z) const override;

    VolumeType GetVolumeType() const noexcept override;

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
     * @param filename descriptor json file path, end with ".raw.desc.json"
     */
    explicit RawGridVolumeReader(const std::string& filename);

    ~RawGridVolumeReader() override;

public:
    void ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, void *buf)             override;

    void ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadFunc reader) override;

    RawGridVolumeDesc GetVolumeDesc() const noexcept override;

protected:
    std::unique_ptr<RawGridVolumeReaderPrivate> _;
};

class RawGridVolumeWriterPrivate;
class RawGridVolumeWriter : public VolumeWriterInterface<RawGridVolumeDesc>{
public:
    explicit RawGridVolumeWriter(const std::string& filename, const RawGridVolumeDesc& desc);

    ~RawGridVolumeWriter() override;

public:
    RawGridVolumeDesc GetVolumeDesc() const noexcept override;

    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const void *buf)        override;

    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeWriteFunc writer) override;

protected:
    std::unique_ptr<RawGridVolumeWriterPrivate> _;
};

enum class SliceAxis : int {
    AXIS_X = 0, AXIS_Y = 1, AXIS_Z = 2
};

struct SlicedGridVolumeDesc : RawGridVolumeDesc{
    SliceAxis axis = SliceAxis::AXIS_Z;
    std::string prefix;
    std::string postfix;
    int setw = 0;
    mutable std::function<std::string(uint32_t)> name_generator;
    bool overwrite = true;
    void Generate() const noexcept {
        name_generator = [this](uint32_t idx){
            std::stringstream ss;
            ss << prefix << std::setw(setw) << std::setfill('0') << std::to_string(idx + 1) << postfix;
            return ss.str();
        };
    }
};

inline bool CheckValidation(SliceAxis axis) noexcept {
    return axis == SliceAxis::AXIS_X
        || axis == SliceAxis::AXIS_Y
        || axis == SliceAxis::AXIS_Z;
}

inline bool CheckValidation(const SlicedGridVolumeDesc& desc) noexcept {
    if(!CheckValidation(static_cast<const RawGridVolumeDesc&>(desc))) return false;
    return CheckValidation(desc.axis) && desc.setw >= 0;
}

class SlicedGridVolumePrivate;
template<typename Voxel>
class SlicedGridVolume :public RawGridVolume<Voxel>{
public:
    using VoxelT = Voxel;
    using SelfT = RawGridVolume<Voxel>;
    static constexpr VolumeType EVolumeType = VolumeType::Grid_SLICED;

    explicit SlicedGridVolume(const SlicedGridVolumeDesc& desc);

    ~SlicedGridVolume() override;

    SlicedGridVolumeDesc GetVolumeDesc() const noexcept;

public:
    void ReadVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, Voxel *buf)                      override;

    void ReadVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, TVolumeReadFunc<Voxel> reader)   override;

    void WriteVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const Voxel *buf)               override;

    void WriteVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, TVolumeWriteFunc<Voxel> writer) override;

    Voxel& operator()(int x, int y, int z) override;

    const Voxel& operator()(int x, int y, int z) const override;

public:
    virtual void ReadSlice(SliceAxis axis, int sliceIndex, void* buf);

    virtual void ReadSlice(int srcX, int srcY, int dstX, int dstY, SliceAxis axis, int sliceIndex, void* buf);

    virtual void WriteSlice(SliceAxis axis, int sliceIndex, const void* buf);

    virtual void WriteSlice(int srcX, int srcY, int dstX, int dstY, SliceAxis axis, int sliceIndex, const void* buf);

protected:
    std::unique_ptr<SlicedGridVolumePrivate> _;
};

using SliceReadFunc = std::function<void(int dx, int dy, const void* src, size_t ele_size)>;
using SliceWriteFunc = std::function<void(int dx, int dy, void* dst, size_t ele_size)>;
class SlicedGridVolumeReaderPrivate;
class SlicedGridVolumeReader : public VolumeReaderInterface<SlicedGridVolumeDesc>{
public:

    SlicedGridVolumeReader(const std::string& filename);

    ~SlicedGridVolumeReader() override;

public:

    void ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, void *buf) override;

    void ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadFunc reader) override;

    SlicedGridVolumeDesc GetVolumeDesc() const noexcept override;

public:
    /**
     * @brief Can only read axis return by GetVolumeDesc, use this if want to read more efficient.
     * Read a region data from slice, the region only require valid which means src < dst but is ok for src < 0 || dst > w/h.
     * Read data will linear fill the buf.
     * @param buf is the address of a linear buffer.
     * @param size buffer length for buf.
     * @return bytes count filled into the buf.
     */
    void ReadSliceData(int sliceIndex, int srcX, int srcY, int dstX, int dstY, void* buf);

    /**
     * @brief This method is mainly to read slice data store/map to a non-linear buffer.
     */
    void ReadSliceData(int sliceIndex, int srcX, int srcY, int dstX, int dstY, SliceReadFunc reader);

    void ReadSliceData(int sliceIndex, void* buf);

    void ReadSliceData(int sliceIndex, SliceReadFunc reader);

    /**
     * @brief If set use cached, reader will first try to read data from cache buffer and read entire slice if can,
     * so it will cost more memory if random access but will be more efficient for read by sequence for huge volume data.
     * @note Internal implement for cache is simple, user can turn off cache and create cache by user self.
     */
    void SetUseCached(bool useCached) noexcept;

    bool GetIfUseCached() const noexcept;

protected:
    std::unique_ptr<SlicedGridVolumeReaderPrivate> _;
};

class SlicedGridVolumeWriterPrivate;
class SlicedGridVolumeWriter : public VolumeWriterInterface<SlicedGridVolumeDesc>{
public:
    SlicedGridVolumeWriter(const std::string& filename, const SlicedGridVolumeDesc& desc);

    ~SlicedGridVolumeWriter() override;

public:
    SlicedGridVolumeDesc GetVolumeDesc() const noexcept override;

    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const void *buf)        override;

    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeWriteFunc writer) override;

public:
    void WriteSliceData(int sliceIndex, const void* buf);

    void WriteSliceData(int sliceIndex, SliceWriteFunc writer);

    void WriteSliceData(int sliceIndex, int srcX, int srcY, int dstX, int dstY, const void* buf);

    void WriteSliceData(int sliceIndex, int srcX, int srcY, int dstX, int dstY, SliceWriteFunc writer);

    void Flush();

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
    friend std::ostream& operator<<(std::ostream& os, const BlockIndex& idx){
        os << "(" << idx.x << ", " << idx.y << ", " << idx.z << ")";
        return os;
    }
};

struct BlockedGridVolumeDesc : RawGridVolumeDesc{
    uint32_t block_length = 0;
    // physical stored block size = block_length + 2 * padding
    uint32_t padding = 0;
};

/**
 * @note This class not implied and not used.
 */
class BlockedGridVolumePrivate;
template <typename Voxel>
class BlockedGridVolume:public RawGridVolume<Voxel>{
public:
    using VoxelT = typename RawGridVolume<Voxel>::VoxelT;

    BlockedGridVolume(const BlockedGridVolumeDesc& desc);

    void ReadBlockData(const BlockIndex&, VoxelT* buf);

    Voxel* GetRawBlockData(const BlockIndex&);

    const Voxel* GetRawBlockData(const BlockIndex&) const;

protected:
    std::unique_ptr<BlockedGridVolumePrivate> _;
};

/**
 * @note This class not implied and not used.
 */
class BlockedGridVolumeReaderPrivate;
class BlockedGridVolumeReader : public VolumeReaderInterface<BlockedGridVolumeDesc>{
public:
    explicit BlockedGridVolumeReader(const std::string& filename);

    ~BlockedGridVolumeReader() override;

public:
    void ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, void *buf)             override;

    void ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadFunc reader) override;

    BlockedGridVolumeDesc GetVolumeDesc() const noexcept override;

public:
    void ReadBlockData(const BlockIndex& blockIndex, void* buf);

    void ReadBlockData(const BlockIndex& blockIndex, VolumeReadFunc reader);

protected:
    std::unique_ptr<BlockedGridVolumeReaderPrivate> _;
};

/**
 * @note This class not implied and not used.
 */
class BlockedGridVolumeWriterPrivate;
class BlockedGridVolumeWriter : public VolumeWriterInterface<BlockedGridVolumeDesc>{
public:
    BlockedGridVolumeWriter(const std::string& filename, const BlockedGridVolumeDesc& desc);

    ~BlockedGridVolumeWriter() override;

public:
    BlockedGridVolumeDesc GetVolumeDesc() const noexcept override;

    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const void *buf)        override;

    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeWriteFunc writer) override;

public:
    void WriteBlockData(const BlockIndex& blockIndex, const void* buf);

    void WriteBlockData(const BlockIndex& blockIndex, VolumeWriteFunc writer);

protected:
    std::unique_ptr<BlockedGridVolumeWriterPrivate> _;
};

enum class GridVolumeCodec: int {
    GRID_VOLUME_CODEC_NONE = 0,
    GRID_VOLUME_CODEC_VIDEO = 1,
    GRID_VOLUME_CODEC_BITS = 2
};

using Packet = std::vector<uint8_t>;
// Packets for store volume video encode results is not perfect, use linear buffer with a packet parser is best.
// But when I realize this is too late... So user would better use a global memory pool will improve efficient.
using Packets = std::vector<Packet>;
using EncodeWorker = std::function<size_t(const void*, Packets&)>;
using DecodeWorker = std::function<size_t(const Packets&, void*)>;

struct EncodedGridVolumeDesc : RawGridVolumeDesc{
    GridVolumeCodec codec;
};

/**
 * @note This class not implied and not used.
 */
template <typename Voxel>
class EncodedGridVolume : public RawGridVolume<Voxel>{
public:

};

/**
 * @note This class not implied and not used.
 */
class EncodedGridVolumeReaderPrivate;
class EncodedGridVolumeReader : public VolumeReaderInterface<EncodedGridVolumeDesc>{
public:
    explicit EncodedGridVolumeReader(const std::string& filename);

    ~EncodedGridVolumeReader() override;

public:
    void ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, void *buf)             override;

    void ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadFunc reader) override;

    EncodedGridVolumeDesc GetVolumeDesc() const noexcept override;

public:
    size_t ReadEncodedData(void* buf, size_t size);

    size_t ReadEncodedData(Packets& packets);

protected:
    std::unique_ptr<EncodedGridVolumeReaderPrivate> _;
};

/**
 * @note This class not implied and not used.
 */
class EncodedGridVolumeWriterPrivate;
class EncodedGridVolumeWriter : public VolumeWriterInterface<EncodedGridVolumeDesc>{
public:
    EncodedGridVolumeWriter(const std::string& filename, const EncodedGridVolumeDesc& desc);

    ~EncodedGridVolumeWriter() override;

public:
    EncodedGridVolumeDesc GetVolumeDesc() const noexcept override;

    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const void *buf)        override;

    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeWriteFunc writer) override;

public:
    void WriteEncodedData(const void* buf, size_t size);

    void WriteEncodedData(const Packets& packets);

protected:
    std::unique_ptr<EncodedGridVolumeWriterPrivate> _;
};

struct EncodedBlockedGridVolumeDesc : BlockedGridVolumeDesc{
    GridVolumeCodec codec;
    char preserve[32];
};

inline bool CheckValidation(const EncodedBlockedGridVolumeDesc& desc){
    if(desc.block_length == 0 || desc.block_length <= (desc.padding << 1)){
        return false;
    }
    if(desc.extend.width == 0 || desc.extend.height == 0 || desc.extend.depth == 0){
        return false;
    }
    return true;
}

/**
 * @note This class not implied and not used.
 */
class EncodedBlockedGridVolumePrivate;
template<typename Voxel>
class EncodedBlockedGridVolume: public BlockedGridVolume<Voxel>{
public:
    explicit EncodedBlockedGridVolume(const EncodedBlockedGridVolumeDesc& desc);

    ~EncodedBlockedGridVolume() override;

public:

protected:
    std::unique_ptr<EncodedBlockedGridVolumePrivate> _;
};


class EncodedBlockedGridVolumeReaderPrivate;
class EncodedBlockedGridVolumeReader : public VolumeReaderInterface<EncodedBlockedGridVolumeDesc>{
public:
    EncodedBlockedGridVolumeReader(const std::string& filename);

    ~EncodedBlockedGridVolumeReader() override;

public:
    void ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, void *buf)             override;

    void ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadFunc reader) override;

    EncodedBlockedGridVolumeDesc GetVolumeDesc() const noexcept override;

public:
    /**
     * @brief Read one block data into buf with size, it will use default(cpu) and suit codec for decoding.
     */
    void ReadBlockData(const BlockIndex& blockIndex, void* buf);

    /**
     * @brief This may be some slow but support more freedom.
     */
    void ReadBlockData(const BlockIndex& blockIndex, VolumeReadFunc reader);

    /**
     * @param size should greater to equal to block bytes.
     * @note read data format is : [(packet_size)(packet_data)][(packet_size)(packet_data)]...
     * @return Exactly filled byte count.
     */
    size_t ReadEncodedBlockData(const BlockIndex& blockIndex, void* buf, size_t size);

    /**
     * @return packed packets bytes
     */
    size_t ReadEncodedBlockData(const BlockIndex& blockIndex, Packets& packets);

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

    ~EncodedBlockedGridVolumeWriter() override;

public:
    EncodedBlockedGridVolumeDesc GetVolumeDesc() const noexcept override;

    /**
     * @brief this method will recover blocks between src and dst region, so tack care of it.
     */
    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const void *buf)        override;

    void WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeWriteFunc writer) override;

public:
    void WriteBlockData(const BlockIndex& blockIndex, const void* buf);

    void WriteBlockData(const BlockIndex& blockIndex, VolumeWriteFunc writer);

    void WriteEncodedBlockData(const BlockIndex& blockIndex, const void* buf, size_t size);

    void WriteEncodedBlockData(const BlockIndex& blockIndex, const Packets& packets);

private:
    std::unique_ptr<EncodedBlockedGridVolumeWriterPrivate> _;
};

enum class CodecDevice {
    CPU, GPU
};

template<typename Voxel,CodecDevice device>
struct VoxelVideoCodec{
    static constexpr bool Value = false;
};

#define Register_VoxelVideoCodec(VoxelT, device) \
template<>                                       \
struct VoxelVideoCodec<VoxelT,device>{           \
    static constexpr bool Value = true;          \
};


template<typename T, CodecDevice device>
inline constexpr bool VoxelVideoCodecV = VoxelVideoCodec<T,device>::Value;

// one voxel type map to one specific codec method and format
Register_VoxelVideoCodec(VoxelRU8,CodecDevice::CPU)
Register_VoxelVideoCodec(VoxelRU8,CodecDevice::GPU)
Register_VoxelVideoCodec(VoxelRU16,CodecDevice::CPU)

class CVolumeCodecInterface{
public:
    virtual ~CVolumeCodecInterface() = default;

    virtual GridVolumeCodec GetCodecType() const noexcept = 0;

    virtual CodecDevice GetCodecDevice() const noexcept = 0;

    /**
     * @note throw on error
     * @note These infos may be not enough so we need derived this interface with template class with Voxel info.
     * @return encoded packets' packed size
     */
    virtual size_t Encode(const Extend3D& extend, const void* buf, size_t size, Packets& packets) = 0;

    /**
     * @note throw on error
     * @note buf' size should be enough large for decoding
     * return decoded buffer size
     */
    virtual size_t Decode(const Extend3D &extend, const Packets &packets, void* buf, size_t size)  = 0;

    //TODO 使用 void* + size_t 替代 Packets
};

template<typename T>
class VolumeCodecInterface : public CVolumeCodecInterface{
public:
    virtual ~VolumeCodecInterface() = default;

    /**
     * @param size buffer's size
     * @return encoded bytes
     */
    virtual size_t Encode(const std::vector<SliceDataView<T>>& slices, void* buf, size_t size)   = 0;

    virtual size_t Encode(const GridDataView<T>& volume, SliceAxis axis, void* buf, size_t size) = 0;

    virtual size_t Encode(const std::vector<SliceDataView<T>> &slices, Packets &packets)         = 0;

    virtual size_t Encode(const GridDataView<T>& volume, SliceAxis axis, Packets &packets)       = 0;

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
class VolumeVideoCodec<T, CodecDevice::CPU, std::enable_if_t<VoxelVideoCodecV<T,CodecDevice::CPU>>>
        : public VolumeVideoCodecInterface<T>{
public:
    explicit VolumeVideoCodec(int threadCount = 1);

    CodecDevice GetCodecDevice() const noexcept override{
        return CodecDevice::CPU;
    }

    size_t Encode(const Extend3D& extend, const void* buf, size_t size, Packets& packets) override;

    size_t Decode(const Extend3D &extend, const Packets &packets, void* buf, size_t size) override;

public:
    size_t Encode(const std::vector<SliceDataView<T>> &slices, void* buf, size_t size)  override;

    size_t Encode(const GridDataView<T>& volume,SliceAxis axis, void* buf, size_t size) override;

    size_t Encode(const std::vector<SliceDataView<T>> &slices, Packets &packets)        override;

    size_t Encode(const GridDataView<T>& volume,SliceAxis axis, Packets &packets)       override;

protected:
    std::unique_ptr<CPUVolumeVideoCodecPrivate> _;
};

template<typename T>
using CPUVolumeVideoCodec = VolumeVideoCodec<T, CodecDevice::CPU, std::enable_if_t<VoxelVideoCodecV<T,CodecDevice::CPU>>>;

class GPUVolumeVideoCodecPrivate;
template<typename T>
class VolumeVideoCodec<T, CodecDevice::GPU, std::enable_if_t<VoxelVideoCodecV<T,CodecDevice::GPU>>>
        : public VolumeVideoCodecInterface<T>{
public:
    explicit VolumeVideoCodec(int GPUIndex = 0);

    /**
     * @param context CUcontext
     */
    explicit VolumeVideoCodec(void* context);

    CodecDevice GetCodecDevice() const noexcept override{
        return CodecDevice::GPU;
    }

    size_t Encode(const Extend3D& extend, const void* buf, size_t size, Packets& packets) override;

    size_t Decode(const Extend3D &extend, const Packets &packets, void* buf, size_t size) override;

public:
    /**
     * @param buf only support cpu ptr now, maybe support device ptr next version...
     */
    size_t Encode(const std::vector<SliceDataView<T>> &slices, void* buf, size_t size)  override;

    size_t Encode(const GridDataView<T>& volume,SliceAxis axis, void* buf, size_t size) override;

    size_t Encode(const std::vector<SliceDataView<T>> &slices, Packets &packets)        override;

    size_t Encode(const GridDataView<T>& volume,SliceAxis axis, Packets &packets)       override;

protected:
    std::unique_ptr<GPUVolumeVideoCodecPrivate> _;
};

template<typename T>
using GPUVolumeVideoCodec = VolumeVideoCodec<T, CodecDevice::GPU, std::enable_if_t<VoxelVideoCodecV<T,CodecDevice::GPU>>>;


struct VolumeFileDesc{
    void Set(VolumeType type, const VolumeFileDesc& desc) noexcept {
        if(type == VolumeType::Grid_RAW)
            raw_desc = desc.raw_desc;
        else if(type == VolumeType::Grid_SLICED)
            sliced_desc = desc.sliced_desc;
        else if(type == VolumeType::Grid_BLOCKED)
            blocked_desc = desc.blocked_desc;
        else if(type == VolumeType::Grid_ENCODED)
            encoded_desc = desc.encoded_desc;
        else if(type == VolumeType::Grid_BLOCKED_ENCODED)
            encoded_blocked_desc = desc.encoded_blocked_desc;
        Set(type);
    }

    void Set(VolumeType vol_type) noexcept {
        this->volume_type = vol_type;
    }

    VolumeType volume_type = VolumeType::Grid_RAW;

    RawGridVolumeDesc raw_desc;
    SlicedGridVolumeDesc sliced_desc;
    BlockedGridVolumeDesc blocked_desc;
    EncodedGridVolumeDesc encoded_desc;
    EncodedBlockedGridVolumeDesc encoded_blocked_desc;
};

inline bool CheckValidation(const VolumeFileDesc& desc, VolumeType type) noexcept {
    switch (type) {
        case VolumeType::Grid_RAW             : return CheckValidation(desc.raw_desc);
        case VolumeType::Grid_SLICED          : return CheckValidation(desc.sliced_desc);
        case VolumeType::Grid_BLOCKED_ENCODED : return CheckValidation(desc.encoded_blocked_desc);
        case VolumeType::Grid_ENCODED         : return CheckValidation(desc.encoded_desc);
        case VolumeType::Grid_BLOCKED         : return CheckValidation(desc.blocked_desc);
        default : return false;
    }
}

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

//histogram
template<VolumeType type>
class VolumeStatistics;

template<>
class VolumeStatistics<VolumeType::Grid_RAW>{
public:
    VolumeStatistics(const std::string& filename, const RawGridVolumeDesc& desc){

    }
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

/**
 * @note This class is not implied and not used.
 */
template<typename Voxel>
class RawGridVolumeIOWrapper : public VolumeIOWrapper<Voxel,VolumeType::Grid_RAW>{
public:
    using BaseT = VolumeIOWrapper<Voxel, VolumeType::Grid_RAW>;
    using VolumeT = typename BaseT::VolumeT;

    static std::unique_ptr<VolumeT> LoadFromMetaFile(const std::string& filename,const RawGridVolumeDesc& desc);

    static void SaveVolumeToMetaFile(const VolumeT& volume, const std::string& filename) noexcept;
};

/**
 * @note This class is not implied and not used.
 */
template<typename Voxel>
class SlicedGridVolumeIOWrapper : public VolumeIOWrapper<Voxel, VolumeType::Grid_SLICED>{
public:
    using BaseT = VolumeIOWrapper<Voxel, VolumeType::Grid_SLICED>;
    using VolumeT = typename BaseT::VolumeT;

    static std::unique_ptr<VolumeT> LoadFromMetaFile(const SlicedGridVolumeDesc& desc);

    static void SaveVolumeToMetaFile(const VolumeT& volume, const std::string& filename) noexcept;
};

/**
 * @note This class is not implied and not used.
 */
template<typename Voxel>
class BlockedGridVolumeIOWrapper : public VolumeIOWrapper<Voxel,VolumeType::Grid_BLOCKED>{
public:
    using BaseT = VolumeIOWrapper<Voxel, VolumeType::Grid_SLICED>;
    using VolumeT = typename BaseT::VolumeT;

    static std::unique_ptr<VolumeT> LoadFromMetaFile();

};

/**
 * @note This class is not implied and not used.
 */
template<typename Voxel>
class EncodedGridVolumeIOWrapper : public VolumeIOWrapper<Voxel,VolumeType::Grid_ENCODED>{
public:
    using BaseT = VolumeIOWrapper<Voxel, VolumeType::Grid_BLOCKED_ENCODED>;
    using VolumeT = typename BaseT::VolumeT;

    static std::unique_ptr<VolumeT> LoadFromMetaFile();

};

/**
 * @note This class is not implied and not used.
 */
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
        friend std::ostream& operator<<(std::ostream& os, const CodecParams& params){
            os << "CodecParams Info :"
            << "\tframe_w : " << params.frame_w
            << "\tframe_h : " << params.frame_h
            << "\tframe_n : " << params.frame_n
            << "\t samplers_per_pixel : " << params.samplers_per_pixel
            << "\t bits_per_sampler : " << params.bits_per_sampler
            << "\t encode : " << params.encode
            << "\t device_index : " << params.device_index
            << "\t context : " << params.context;
            return os;
        }
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

    /**
     * @param buf start ptr for buffer to decode into
     * @param size buffer size for buf, must be large enough
     * @return decoded size for decoding
     */
    virtual size_t DecodePacketIntoFrames(const Packet& packet, void* buf, size_t size) = 0;
};

class CPUVolumeVideoCodecPrivate{
public:
    std::unique_ptr<VideoCodec> video_codec;
    int thread_count = 0;
};

template<typename T>
CPUVolumeVideoCodec<T>::VolumeVideoCodec(int threadCount) {
    _ = std::make_unique<CPUVolumeVideoCodecPrivate>();
    _->video_codec = VideoCodec::Create(CodecDevice::CPU);
    _->thread_count = threadCount;
}

// ===================
// 主要使用的两个接口函数
template<typename T>
size_t CPUVolumeVideoCodec<T>::Encode(const Extend3D &extend, const void *buf, size_t size, Packets &packets) {
    auto [w, h, d] = extend;
    VideoCodec::CodecParams params{
            .frame_w = (int)w,
            .frame_h = (int)h,
            .frame_n = (int)d,
            .samplers_per_pixel = GetVoxelSampleCount(T::format),
            .bits_per_sampler = GetVoxelBits(T::type),
            .threads_count = _->thread_count
    };
    if(!_->video_codec->ReSet(params)){
        std::cerr << "Invalid Video CodecParams, " << params << std::endl;
        throw VolumeCodecError("CPU video encode reset failed");
    }

    auto src_ptr = reinterpret_cast<const uint8_t*>(buf);
    auto voxel_size = GetVoxelSize(T::type, T::format);
    const size_t slice_size = w * h * voxel_size;
    assert(size == (size_t)w * h * d * voxel_size);
    size_t packed_size = 0;
    for(int z = 0; z < d + 1; z++){
        Packets tmp_packets;
        if(z < d){
            size_t src_offset = (size_t)z * slice_size;
            _->video_codec->EncodeFrameIntoPackets(src_ptr + src_offset, slice_size, tmp_packets);
        }
        else
            // one more for end
            _->video_codec->EncodeFrameIntoPackets(nullptr, 0, tmp_packets);
        for(auto& p : tmp_packets)
            packed_size += p.size() + sizeof(size_t);
        packets.insert(packets.end(), tmp_packets.begin(), tmp_packets.end());
    }
    return packed_size;
}

template<typename T>
size_t CPUVolumeVideoCodec<T>::Decode(const Extend3D &extend, const Packets &packets, void *buf, size_t size) {
    assert(buf && size && !packets.empty());

    auto voxel_size = GetVoxelSize(T::type, T::format);
    assert(extend.size() * voxel_size <= size);

    VideoCodec::CodecParams params{
        .threads_count = _->thread_count,
        .encode = false
    };
    if(!_->video_codec->ReSet(params)){
        std::cerr << "Invalid Video CodecParams, " << params << std::endl;
        throw VolumeCodecError("CPU video decode reset failed");
    }

    auto dst_ptr = reinterpret_cast<uint8_t*>(buf);
    size_t decode_size = 0;
    for(auto& packet : packets){
        auto ret = _->video_codec->DecodePacketIntoFrames(packet, dst_ptr + decode_size, size - decode_size);
        decode_size += ret;
    }
    // one more for end
    auto ret = _->video_codec->DecodePacketIntoFrames({}, dst_ptr + decode_size, size - decode_size);
    decode_size += ret;
    if(decode_size > size){
        throw VolumeCodecError("CPU volume video decode error : target decode buffer size is not enough large!");
    }
    return decode_size;
}
// ===================

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
    GridData<T> grid(volume.Width(), volume.Height(), volume.Depth());
    if(axis == SliceAxis::AXIS_Z){
        for(uint32_t z = 0; z < grid.Depth(); z++){
            for(uint32_t y = 0; y < grid.Height(); y++){
                for(uint32_t x = 0; x < grid.Width(); x++){
                    grid(x, y, z) = volume(x, y, z);
                }
            }
        }
    }
    else if(axis == SliceAxis::AXIS_Y){
        // x y z -> z x y
        for(uint32_t z = 0; z < grid.Depth(); z++){
            for(uint32_t y = 0; y < grid.Height(); y++){
                for(uint32_t x = 0; x < grid.Width(); x++){
                    grid(z, x, y) = volume(x, y, z);
                }
            }
        }
    }
    else if(axis == SliceAxis::AXIS_X){
        // x y z - > y z x
        for(uint32_t z = 0; z < grid.Depth(); z++){
            for(uint32_t y = 0; y < grid.Height(); y++){
                for(uint32_t x = 0; x < grid.Width(); x++){
                    grid(y, z, x) = volume(x, y, z);
                }
            }
        }
    }
    Packets packets;
    auto ret = Encode({grid.Width(), grid.Height(), grid.Depth()}, grid.GetRawPtr(), grid.Size() * sizeof(T), packets);
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
size_t CPUVolumeVideoCodec<T>::Encode(const std::vector<SliceDataView<T>> &slices, Packets &packets) {
    // SliceDataView' storage maybe not linear because of map
    auto voxel_size = T::size();
    auto width = slices.front().Width();
    auto height = slices.front().Height();
    auto depth = slices.size();
    auto slice_size = voxel_size * width * height;
    VideoCodec::CodecParams params{
            (int)width,(int)height,(int)depth,
            GetVoxelSampleCount(T::format),
            GetVoxelBits(T::type),
            _->thread_count
    };
    if(!_->video_codec->ReSet(params)) return false;
    SliceData<T> slice_data(width, height);
    for(auto& slice : slices){
        Packets tmp_packets;
        if(slice.IsLinear()){
            _->video_codec->EncodeFrameIntoPackets(slice.GetRawPtr(), slice_size, tmp_packets);
        }
        else{
            for(int i = 0; i < height; i++)
                for(int j = 0; j < width; j++)
                    slice_data(j, i) = slice(j, i);
            _->video_codec->EncodeFrameIntoPackets(slice_data.GetRawPtr(), slice_size, tmp_packets);
        }
        packets.insert(packets.end(), tmp_packets.begin(), tmp_packets.end());
    }
    //one more for end
    Packets tmp_packets;
    _->video_codec->EncodeFrameIntoPackets(nullptr, 0, tmp_packets);
    packets.insert(packets.end(), tmp_packets.begin(), tmp_packets.end());
    return true;
}

template<typename T>
size_t CPUVolumeVideoCodec<T>::Encode(const GridDataView<T> &volume, SliceAxis axis, Packets &packets) {
    GridData<T> grid(volume.Width(), volume.Height(), volume.Depth());
    if(axis == SliceAxis::AXIS_Z){
        for(uint32_t z = 0; z < grid.Depth(); z++){
            for(uint32_t y = 0; y < grid.Height(); y++){
                for(uint32_t x = 0; x < grid.Width(); x++){
                    grid(x, y, z) = volume(x, y, z);
                }
            }
        }
    }
    else if(axis == SliceAxis::AXIS_Y){
        // x y z -> z x y
        for(uint32_t z = 0; z < grid.Depth(); z++){
            for(uint32_t y = 0; y < grid.Height(); y++){
                for(uint32_t x = 0; x < grid.Width(); x++){
                    grid(z, x, y) = volume(x, y, z);
                }
            }
        }
    }
    else if(axis == SliceAxis::AXIS_X){
        // x y z - > y z x
        for(uint32_t z = 0; z < grid.Depth(); z++){
            for(uint32_t y = 0; y < grid.Height(); y++){
                for(uint32_t x = 0; x < grid.Width(); x++){
                    grid(y, z, x) = volume(x, y, z);
                }
            }
        }
    }
    auto ret = Encode({grid.Width(), grid.Height(), grid.Depth()}, grid.GetRawPtr(), grid.Size() * sizeof(T), packets);
    return ret > 0;
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
size_t GPUVolumeVideoCodec<T>::Encode(const Extend3D &extend, const void *buf, size_t size, Packets &packets) {
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
    const size_t slice_size = w * h * voxel_size;
    assert(size == slice_size * d);
    for(int z = 0; z < d + 1; z++){
        Packets tmp_packets;
        if(z < d)
            _->video_codec->EncodeFrameIntoPackets(data_view.ViewSliceZ(z).data, slice_size, tmp_packets);
        else
            _->video_codec->EncodeFrameIntoPackets(nullptr, 0, tmp_packets);
        packets.insert(packets.end(), tmp_packets.begin(), tmp_packets.end());
    }
    return true;
}

template<typename T>
size_t GPUVolumeVideoCodec<T>::Decode(const Extend3D &extend, const Packets &packets, void *buf, size_t size) {
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
    auto ret = _->video_codec->DecodePacketIntoFrames({}, dst_ptr + decode_size, size - decode_size);
    decode_size += ret;
    return true;
}

template<typename T>
size_t GPUVolumeVideoCodec<T>::Encode(const std::vector<SliceDataView<T>> &slices, void *buf, size_t size) {
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
size_t GPUVolumeVideoCodec<T>::Encode(const GridDataView<T> &volume, SliceAxis axis, void *buf, size_t size) {
    GridData<T> grid(volume.Width(), volume.Height(), volume.Depth());
    if(axis == SliceAxis::AXIS_Z){
        for(uint32_t z = 0; z < grid.Depth(); z++){
            for(uint32_t y = 0; y < grid.Height(); y++){
                for(uint32_t x = 0; x < grid.Width(); x++){
                    grid(x, y, z) = volume(x, y, z);
                }
            }
        }
    }
    else if(axis == SliceAxis::AXIS_Y){
        // x y z -> z x y
        for(uint32_t z = 0; z < grid.Depth(); z++){
            for(uint32_t y = 0; y < grid.Height(); y++){
                for(uint32_t x = 0; x < grid.Width(); x++){
                    grid(z, x, y) = volume(x, y, z);
                }
            }
        }
    }
    else if(axis == SliceAxis::AXIS_X){
        // x y z - > y z x
        for(uint32_t z = 0; z < grid.Depth(); z++){
            for(uint32_t y = 0; y < grid.Height(); y++){
                for(uint32_t x = 0; x < grid.Width(); x++){
                    grid(y, z, x) = volume(x, y, z);
                }
            }
        }
    }
    Packets packets;
    auto ret = Encode({grid.Width(), grid.Height(), grid.Depth()}, grid.GetRawPtr(), grid.Size() * sizeof(T), packets);
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
size_t GPUVolumeVideoCodec<T>::Encode(const std::vector<SliceDataView<T>> &slices, Packets &packets) {
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
    SliceData<T> slice_data(width, height);
    for(auto& slice : slices){
        Packets tmp_packets;
        if(slice.IsLinear()){
            _->video_codec->EncodeFrameIntoPackets(slice.GetRawPtr(), slice_size, tmp_packets);
        }
        else{
            for(int i = 0; i < height; i++)
                for(int j = 0; j < width; j++)
                    slice_data(j, i) = slice(j, i);
            _->video_codec->EncodeFrameIntoPackets(slice_data.GetRawPtr(), slice_size, tmp_packets);
        }
        packets.insert(packets.end(), tmp_packets.begin(), tmp_packets.end());
    }
    //one more for end
    Packets tmp_packets;
    _->video_codec->EncodeFrameIntoPackets(nullptr, 0, tmp_packets);
    packets.insert(packets.end(), tmp_packets.begin(), tmp_packets.end());
    return true;
}

template<typename T>
size_t GPUVolumeVideoCodec<T>::Encode(const GridDataView<T> &volume, SliceAxis axis, Packets &packets) {
    GridData<T> grid(volume.Width(), volume.Height(), volume.Depth());
    if(axis == SliceAxis::AXIS_Z){
        for(uint32_t z = 0; z < grid.Depth(); z++){
            for(uint32_t y = 0; y < grid.Height(); y++){
                for(uint32_t x = 0; x < grid.Width(); x++){
                    grid(x, y, z) = volume(x, y, z);
                }
            }
        }
    }
    else if(axis == SliceAxis::AXIS_Y){
        // x y z -> z x y
        for(uint32_t z = 0; z < grid.Depth(); z++){
            for(uint32_t y = 0; y < grid.Height(); y++){
                for(uint32_t x = 0; x < grid.Width(); x++){
                    grid(z, x, y) = volume(x, y, z);
                }
            }
        }
    }
    else if(axis == SliceAxis::AXIS_X){
        // x y z - > y z x
        for(uint32_t z = 0; z < grid.Depth(); z++){
            for(uint32_t y = 0; y < grid.Height(); y++){
                for(uint32_t x = 0; x < grid.Width(); x++){
                    grid(y, z, x) = volume(x, y, z);
                }
            }
        }
    }
    auto ret = Encode({grid.Width(), grid.Height(), grid.Depth()}, grid.GetRawPtr(), grid.Size() * sizeof(T), packets);
    return ret > 0;
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