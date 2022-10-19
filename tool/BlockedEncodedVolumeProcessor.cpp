#include "VolumeProcessor.hpp"
#include "Detail/IOImpl.hpp"
#include <iostream>

template<typename Voxel>
class BlockedEncodedVolumeProcessorPrivate{
public:
    using Self = SlicedVolumeProcessorPrivate<Voxel>;
    using Unit = typename Processor<Voxel>::Unit;
    using UnitDSFunc = typename DownSamplingOp<Voxel>::Func;
    using UnitMPFunc = typename MappingOp<Voxel>::Func;
    using UnitSSFunc = StatisticsOp<Voxel>;
    std::unique_ptr<EncodedBlockedGridVolumeReader> encoded_blocked_reader;
    Unit src;
    std::unordered_map<VolumeType, std::queue<Unit>> unit_mp;
    int type_mask;
    VolumeRange range;

    void Init(){
        encoded_blocked_reader = std::make_unique<EncodedBlockedGridVolumeReader>(src.desc_filename);
    }

    void Reset(){
        encoded_blocked_reader.reset();
        unit_mp.clear();
        type_mask = 0;
        range = {0, 0, 0, 0, 0, 0};
    }

    template<VolumeType type, VolumeType... types>
    void Convert(){
        Convert<type>();
        Convert<types...>();
    }

    template<>
    void Convert<VolumeType::Grid_RAW>(){
        assert(unit_mp.count(VolumeType::Grid_RAW));
        auto& units = unit_mp[VolumeType::Grid_RAW];

        typename IOImpl<Voxel>::PackedParams1 packed = {
                .reader = encoded_blocked_reader.get(),
                .range = range,
        };
        std::vector<std::unique_ptr<RawGridVolumeWriter>> writers;
        while(!units.empty()){
            auto unit = units.front();
            units.pop();
            // init writer
            assert(unit.type == VolumeType::Grid_RAW);

            auto& writer = writers.emplace_back(std::make_unique<RawGridVolumeWriter>(unit.desc_filename, unit.desc.raw_desc));

            int op_mask = unit.ops.op_mask;
            bool has_mp = op_mask & Mapping;
            bool has_ds = op_mask & DownSampling;
            bool has_ss = op_mask & Statistics;

            auto ss = std::make_shared<StatisticsOp<Voxel>>();
            auto down_sampling_func = unit.ops.down_sampling.GetOp();
            auto mapping_func = unit.ops.mapping.GetOp();

            packed.writers.push_back({
                                             .other_writer = writer.get(),
                                             .other_has_ds = has_ds,
                                             .other_has_mp = has_mp,
                                             .other_has_ss = has_ss,
                                             .other_ds_func = down_sampling_func,
                                             .other_mp_func = mapping_func,
                                             .other_ss_func = ss
                                     });
        }
        IOImpl<Voxel>::ConvertRawOrSlicedImpl(packed);
    }

    template<>
    void Convert<VolumeType::Grid_SLICED>(){
        assert(unit_mp.count(VolumeType::Grid_SLICED));
        auto& units = unit_mp[VolumeType::Grid_SLICED];
        int slice_w = range.dst_x - range.src_x;
        int slice_h = range.dst_y - range.src_y;

        typename IOImpl<Voxel>::PackedParams1 packed = {
                .reader = encoded_blocked_reader.get(),
                .range = range,
        };
        std::vector<std::unique_ptr<SlicedGridVolumeWriter>> writers;
        while(!units.empty()){
            auto unit = units.front();
            units.pop();
            // init writer
            assert(unit.type == VolumeType::Grid_SLICED);

            auto& writer = writers.emplace_back(std::make_unique<SlicedGridVolumeWriter>(unit.desc_filename, unit.desc.sliced_desc));

            int op_mask = unit.ops.op_mask;
            bool has_mp = op_mask & Mapping;
            bool has_ds = op_mask & DownSampling;
            bool has_ss = op_mask & Statistics;

            auto ss = std::make_shared<StatisticsOp<Voxel>>();
            auto down_sampling_func = unit.ops.down_sampling.GetOp();
            auto mapping_func = unit.ops.mapping.GetOp();

            packed.writers.push_back({
                                             .other_writer = writer.get(),
                                             .other_has_ds = has_ds,
                                             .other_has_mp = has_mp,
                                             .other_has_ss = has_ss,
                                             .other_ds_func = down_sampling_func,
                                             .other_mp_func = mapping_func,
                                             .other_ss_func = ss
                                     });
        }
        IOImpl<Voxel>::ConvertRawOrSlicedImpl(packed);
    }

    template<>
    void Convert<VolumeType::Grid_BLOCKED_ENCODED>(){
        auto oblocked_encoded_unit = unit_mp[VolumeType::Grid_BLOCKED_ENCODED].front();
        unit_mp[VolumeType::Grid_BLOCKED_ENCODED].pop();
        typename IOImpl<Voxel>::PackedParams0 packed = {
                .reader = encoded_blocked_reader.get(),
                .range = range,
                .oblocked_encoded_unit = oblocked_encoded_unit
        };
        IOImpl<Voxel>::template ConvertBlockedEncodedImpl<false>(packed);
    }


    std::unordered_map<int, std::function<void()>> mask_func;

    BlockedEncodedVolumeProcessorPrivate(){
        mask_func[Grid_RAW_BIT] = [this]{ Convert<VolumeType::Grid_RAW>(); };
        mask_func[Grid_SLICED_BIT] = [this]{ Convert<VolumeType::Grid_SLICED>(); };
        mask_func[Grid_BLOCKED_ENCODED_BIT] = [this]{ Convert<VolumeType::Grid_BLOCKED_ENCODED>(); };

    }
};


template<typename Voxel>
VolumeProcessor<Voxel, VolumeType::Grid_BLOCKED_ENCODED>::VolumeProcessor() {
    _ = std::make_unique<BlockedEncodedVolumeProcessorPrivate<Voxel>>();
}

template<typename Voxel>
VolumeProcessor<Voxel, VolumeType::Grid_BLOCKED_ENCODED>::~VolumeProcessor() {

}

template<typename Voxel>
VolumeProcessor<Voxel, VolumeType::Grid_BLOCKED_ENCODED>&
VolumeProcessor<Voxel, VolumeType::Grid_BLOCKED_ENCODED>::SetSource(const Unit& u, const VolumeRange& range){
    assert(u.type == VolumeType::Grid_BLOCKED_ENCODED);
//    if(!CheckValidation(u.desc.encoded_blocked_desc)){
//        std::cerr << "Invalid source volume desc for encoded blocked volume" << std::endl;
//        return *this;
//    }
    _->Reset();
    _->src = u;
    _->range = range;
    _->Init();
    return *this;
}

template<typename Voxel>
VolumeProcessor<Voxel, VolumeType::Grid_BLOCKED_ENCODED>&
VolumeProcessor<Voxel, VolumeType::Grid_BLOCKED_ENCODED>::AddTarget(const Unit& u){
    if(!CheckValidation(u.desc, u.type)){
        std::cerr << "Invalid target volume desc" << std::endl;
        return *this;
    }

    _->type_mask |= GetVolumeTypeBits(u.type);
    _->unit_mp[u.type].push(u);

    return *this;
}

template<typename Voxel>
void VolumeProcessor<Voxel, VolumeType::Grid_BLOCKED_ENCODED>::Convert(){
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
    _->type_mask = 0;
}

template class VolumeProcessor<VoxelRU8, vol::VolumeType::Grid_BLOCKED_ENCODED>;
template class VolumeProcessor<VoxelRU16, vol::VolumeType::Grid_BLOCKED_ENCODED>;

