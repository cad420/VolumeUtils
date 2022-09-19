#include "VolumeProcessor.hpp"
#include <iostream>

template<typename Voxel>
class RawVolumeProcessorPrivate{
public:
    using Unit = typename Processor<Voxel>::Unit;
    std::unique_ptr<RawGridVolumeReader> raw_reader;
    Unit src;
    std::unordered_map<VolumeType, std::vector<Unit>> unit_mp;
    int type_mask;
    VolumeRange range;
    

    template<VolumeType... types>
    void Convert();

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
    void Convert<VolumeType::Grid_RAW,VolumeType::Grid_SLICED>(){

    }

    std::unordered_map<int, std::function<void()>> mask_func;

    RawVolumeProcessorPrivate(){
        mask_func[Grid_RAW_BIT] = [this]{ Convert<VolumeType::Grid_RAW>(); };
        mask_func[Grid_SLICED_BIT] = [this]{ Convert<VolumeType::Grid_SLICED>(); };
        mask_func[Grid_BLOCKED_ENCODED_BIT] = [this]{ Convert<VolumeType::Grid_BLOCKED_ENCODED>(); };
        mask_func[Grid_SLICED_BIT | Grid_BLOCKED_ENCODED_BIT] = [this]{
            Convert<VolumeType::Grid_SLICED, VolumeType::Grid_BLOCKED_ENCODED>();
        };
    }
};

template<typename Voxel>
VolumeProcessor<Voxel, VolumeType::Grid_RAW>::VolumeProcessor() {

}

template<typename Voxel>
VolumeProcessor<Voxel, VolumeType::Grid_RAW>::~VolumeProcessor() {

}

template<typename Voxel>
VolumeProcessor<Voxel, VolumeType::Grid_RAW>&
VolumeProcessor<Voxel, VolumeType::Grid_RAW>::SetSource(const Unit& u, const VolumeRange& range) {
    assert(u.type == VolumeType::Grid_RAW);
    if(!CheckValidation(u.desc.raw_desc)){
        std::cerr << "Invalid source volume desc" << std::endl;
        return *this;
    }
    _->src = u;
    _->range = range;

    return *this;
}

template<typename Voxel>
VolumeProcessor<Voxel, VolumeType::Grid_RAW>&
VolumeProcessor<Voxel, VolumeType::Grid_RAW>::AddTarget(const Unit& u) {
    if(!CheckValidation(u.desc, u.type)){
        std::cerr << "Invalid target volume desc" << std::endl;
        return *this;
    }
    _->type_mask |= GetVolumeTypeBits(u.type);
    _->unit_mp[u.type].push_back(u);

    return *this;
}

template<typename Voxel>
void VolumeProcessor<Voxel, VolumeType::Grid_RAW>::Convert() {
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

template class VolumeProcessor<VoxelRU8, vol::VolumeType::Grid_RAW>;
