#include "VolumeProcessor.hpp"
#include <iostream>

template<typename Voxel>
class SlicedVolumeProcessorPrivate{
public:
    using Self = SlicedVolumeProcessorPrivate<Voxel>;
    using Unit = typename Processor<Voxel>::Unit;
    std::unique_ptr<SlicedGridVolumeReader> sliced_reader;
    Unit src;
    std::unordered_map<VolumeType, std::vector<Unit>> unit_mp;
    int type_mask;
    VolumeRange range;


    template<VolumeType... types>
    void Convert(){

    }

    template<>
    void Convert<VolumeType::Grid_RAW>(){

    }
    template<>
    void Convert<VolumeType::Grid_SLICED>(){

    }
    template<>
    void Convert<VolumeType::Grid_BLOCKED_ENCODED>(){

    }
    template<>
    void Convert<VolumeType::Grid_SLICED, VolumeType::Grid_BLOCKED_ENCODED>(){

    }

    std::unordered_map<int, std::function<void()>> mask_func;

    SlicedVolumeProcessorPrivate(){
        mask_func[Grid_RAW_BIT] = [this]{ Convert<VolumeType::Grid_RAW>(); };
        mask_func[Grid_SLICED_BIT] = [this]{ Convert<VolumeType::Grid_SLICED>(); };
        mask_func[Grid_BLOCKED_ENCODED_BIT] = [this]{ Convert<VolumeType::Grid_BLOCKED_ENCODED>(); };
        mask_func[Grid_SLICED_BIT | Grid_BLOCKED_ENCODED_BIT] = [this]{
            Convert<VolumeType::Grid_SLICED, VolumeType::Grid_BLOCKED_ENCODED>();
        };
    }
};

template<typename Voxel>
VolumeProcessor<Voxel, VolumeType::Grid_SLICED>::VolumeProcessor() {
    _ = std::make_unique<SlicedVolumeProcessorPrivate<Voxel>>();
}

template<typename Voxel>
VolumeProcessor<Voxel, VolumeType::Grid_SLICED>::~VolumeProcessor() {

}

template<typename Voxel>
VolumeProcessor<Voxel, VolumeType::Grid_SLICED>& VolumeProcessor<Voxel, VolumeType::Grid_SLICED>::SetSource(const Unit& u, const VolumeRange& range){
    assert(u.type == VolumeType::Grid_SLICED);
    if(!CheckValidation(u.desc.sliced_desc)){
        std::cerr << "Invalid source volume desc" << std::endl;
        return *this;
    }
    _->src = u;
    _->range = range;
    return *this;
}

template<typename Voxel>
VolumeProcessor<Voxel, VolumeType::Grid_SLICED>& VolumeProcessor<Voxel, VolumeType::Grid_SLICED>::AddTarget(const Unit& u){
    if(!CheckValidation(u.desc, u.type)){
        std::cerr << "Invalid target volume desc" << std::endl;
        return *this;
    }
    _->type_mask |= GetVolumeTypeBits(u.type);
    _->unit_mp[u.type].push_back(u);

    return *this;
}

template<typename Voxel>
void VolumeProcessor<Voxel, VolumeType::Grid_SLICED>::Convert(){
    if(_->mask_func.count(_->type_mask) == 0){
        std::cerr<< "Convert function not imply..." << std::endl;
        return;
    }
    try {
        _->mask_func[_->type_mask]();
    }
    catch (const std::exception& e) {
        std::cerr<<" Convert failed : " << e.what() << std::endl;
    }
}

template class VolumeProcessor<VoxelRU8, vol::VolumeType::Grid_SLICED>;