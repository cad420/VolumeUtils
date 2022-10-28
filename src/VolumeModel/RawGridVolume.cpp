#include <VolumeUtils/Volume.hpp>

#include "../Common/Utils.hpp"
#include "../Common/Common.hpp"

VOL_BEGIN

class RawGridVolumePrivate{
public:
    void* ptr = nullptr;
    std::vector<uint8_t> buffer;
    size_t size = 0;

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 0;

    RawGridVolumeDesc desc;

    ~RawGridVolumePrivate(){

    }

    void Init(){
        width = desc.extend.width;
        height = desc.extend.height;
        depth = desc.extend.depth;

        size = GetVoxelSize(desc.voxel_info) * width * height * depth;

        buffer.resize(size, 0);
        ptr = buffer.data();
        if(ptr == nullptr){
            throw std::runtime_error("Alloc RawGridVolume failed with size : " + std::to_string(size));
        }
    }

    /**
     * @param offset count in byte
     */
    void* GetPtr(size_t offset = 0){
        assert(offset < size);
        return reinterpret_cast<uint8_t*>(ptr) + offset;
    }

};

template<typename Voxel>
RawGridVolume<Voxel>::RawGridVolume(const RawGridVolumeDesc& desc){
    if(!CheckValidation(desc)){
        PrintVolumeDesc(desc);
        throw VolumeFileContextError("RawGridVolumeDesc is invalid for create RawGridVolume");
    }
    _ = std::make_unique<RawGridVolumePrivate>();
    _->desc = desc;
    _->Init();
}

template<typename Voxel>
RawGridVolume<Voxel>::~RawGridVolume() {

}

template<typename Voxel>
void RawGridVolume<Voxel>::ReadVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, Voxel *buf) {
    assert(buf && srcX < dstX && srcY < dstY && srcZ < dstZ);
#ifndef HIGH_PERFORMANCE
    return ReadVoxels(srcX, srcY, srcZ, dstX, dstY, dstZ,
                      [len_x = dstX - srcX, len_y = dstY - srcY, buf]
                      (int dx, int dy, int dz, const Voxel& voxel)->size_t{
        size_t dst_offset = (size_t)dz * len_y * len_x + (size_t)dy * len_x + dx;
        buf[dst_offset] = voxel;
        return 1;
    });
#else
    int beg_z = std::max<int>(srcZ, 0), end_z = std::min<int>(dstZ, _->depth);
    int beg_y = std::max<int>(srcY, 0), end_y = std::min<int>(dstY, _->height);
    int beg_x = std::max<int>(srcX, 0), end_x = std::min<int>(dstX, _->width);
    int len_y = dstY - srcY, len_x = dstX - srcX;
    for(int z = beg_z; z < end_z; z++){
        for(int y = beg_y; y < end_y; y++){
            for(int x = beg_x; x < end_x; x++){
                size_t offset = (size_t)(z - srcZ) * len_x * len_y + (size_t)(y - srcY) * len_x + x - srcX;
                buf[offset] = this->operator()(x, y, z);
            }
        }
    }
#endif
}

template<typename Voxel>
void RawGridVolume<Voxel>::ReadVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, TVolumeReadFunc<Voxel> reader) {
    assert(reader && srcX < dstX && srcY < dstY && srcZ < dstZ);
    int beg_z = std::max<int>(srcZ, 0), end_z = std::min<int>(dstZ, _->depth);
    int beg_y = std::max<int>(srcY, 0), end_y = std::min<int>(dstY, _->height);
    int beg_x = std::max<int>(srcX, 0), end_x = std::min<int>(dstX, _->width);
    for(int z = beg_z; z < end_z; z++){
        for(int y = beg_y; y < end_y; y++){
            for(int x = beg_x; x < end_x; x++){
                reader(x - srcX, y - srcY, z - srcZ, this->operator()(x, y, z));
            }
        }
    }
}

template<typename Voxel>
void RawGridVolume<Voxel>::WriteVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const Voxel *buf) {
    assert(buf && srcX < dstX && srcY < dstY && srcZ < dstZ);
#ifndef HIGH_PERFORMANCE
    return WriteVoxels(srcX, srcY, srcZ, dstX, dstY, dstZ,
                       [len_x = dstX - srcX, len_y = dstY - srcY, buf]
                       (int dx, int dy, int dz, Voxel& dst){
        size_t src_offset = (size_t)dz * len_y * len_x + (size_t)dy * len_x + dx;
        dst = buf[src_offset];
    });
#else
    int beg_z = std::max<int>(srcZ, 0), end_z = std::min<int>(dstZ, _->depth);
    int beg_y = std::max<int>(srcY, 0), end_y = std::min<int>(dstY, _->height);
    int beg_x = std::max<int>(srcX, 0), end_x = std::min<int>(dstX, _->width);
    int len_y = dstY - srcY, len_x = dstX - srcX;
    for(int z = beg_z; z < end_z; z++){
        for(int y = beg_y; y < end_y; y++){
            for(int x = beg_x; x < end_x; x++){
                size_t offset = (size_t)(z - srcZ) * len_x * len_y + (size_t)(y - srcY) * len_x + x - srcX;
                this->operator()(x, y, z) = buf[offset];
            }
        }
    }
#endif
}

template<typename Voxel>
void RawGridVolume<Voxel>::WriteVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, TVolumeWriteFunc<Voxel> writer) {
    assert(writer && srcX < dstX && srcY < dstY && srcZ < dstZ);
    int beg_z = std::max<int>(srcZ, 0), end_z = std::min<int>(dstZ, _->depth);
    int beg_y = std::max<int>(srcY, 0), end_y = std::min<int>(dstY, _->height);
    int beg_x = std::max<int>(srcX, 0), end_x = std::min<int>(dstX, _->width);
    for(int z = beg_z; z < end_z; z++){
        for(int y = beg_y; y < end_y; y++){
            for(int x = beg_x; x < end_x; x++){
                writer(x - srcX, y - srcY, z - srcZ, this->operator()(x, y, z));
            }
        }
    }
}

template<typename Voxel>
Voxel& RawGridVolume<Voxel>::operator()(int x, int y, int z) {
    assert(x >= 0 && y >= 0 && z >= 0 && x < _->width && y < _->height && z < _->depth);
    size_t idx = (size_t)z * _->width * _->height + (size_t)y * _->width + x;
    return *static_cast<Voxel*>(_->GetPtr(idx * sizeof(Voxel)));
}

template<typename Voxel>
const Voxel& RawGridVolume<Voxel>::operator()(int x, int y, int z) const {
    assert(x >= 0 && y >= 0 && z >= 0 && x < _->width && y < _->height && z < _->depth);
    size_t idx = (size_t)z * _->width * _->height + (size_t)y * _->width + x;
    return *static_cast<Voxel*>(_->GetPtr(idx * sizeof(Voxel)));
}

template<typename Voxel>
VolumeType RawGridVolume<Voxel>::GetVolumeType() const noexcept {
    return VolumeType::Grid_RAW;
}

template<typename Voxel>
Voxel* RawGridVolume<Voxel>::GetRawDataPtr() noexcept {
    return static_cast<Voxel*>(_->GetPtr());
}

template<typename Voxel>
const Voxel* RawGridVolume<Voxel>::GetRawDataPtr() const noexcept {
    return static_cast<Voxel*>(_->GetPtr());
}

template<typename Voxel>
RawGridVolumeDesc RawGridVolume<Voxel>::GetVolumeDesc() const noexcept {
    return _->desc;
}

template class RawGridVolume<VoxelRU8>;
template class RawGridVolume<VoxelRU16>;

VOL_END