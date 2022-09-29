#include <VolumeUtils/Volume.hpp>

VOL_BEGIN

class RawGridVolumePrivate{
public:

    void* ptr = nullptr;
    size_t size = 0;

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 0;

    void* GetPtr(size_t offset = 0){
        assert(offset < size);
        return reinterpret_cast<uint8_t*>(ptr) + size;
    }

};

template<typename Voxel>
RawGridVolume<Voxel>::RawGridVolume(const RawGridVolumeDesc& desc){

}

template<typename Voxel>
RawGridVolume<Voxel>::~RawGridVolume() {

}

template<typename Voxel>
size_t RawGridVolume<Voxel>::ReadVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, Voxel *buf,
                                        size_t size) noexcept {
    return 0;
}

template<typename Voxel>
size_t RawGridVolume<Voxel>::ReadVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, Voxel *buf,
                                        std::function<Voxel(const Voxel &)> mapper) noexcept {
    return 0;
}

template<typename Voxel>
size_t
RawGridVolume<Voxel>::WriteVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const Voxel *buf,
                                  size_t size) noexcept {
    return 0;
}

template<typename Voxel>
size_t RawGridVolume<Voxel>::WriteVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, Voxel *buf,
                                         std::function<Voxel(const Voxel &)> mapper) noexcept {
    return 0;
}

template<typename Voxel>
Voxel &RawGridVolume<Voxel>::operator()(int x, int y, int z) {
    size_t idx = (size_t)z * _->width * _->height + (size_t)y * _->width + x;
    return *static_cast<Voxel*>(_->GetPtr(idx * sizeof(Voxel)));
}

template<typename Voxel>
const Voxel &RawGridVolume<Voxel>::operator()(int x, int y, int z) const {
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


template class RawGridVolume<VoxelRU8>;
template class RawGridVolume<VoxelRU16>;

VOL_END