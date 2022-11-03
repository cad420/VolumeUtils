#include "VolumeProcessor.hpp"
#include "Detail/IOImpl.hpp"
#include <iostream>
#include "Common.hpp"
template<typename Voxel>
class RawVolumeProcessorPrivate{
public:
    using Self = SlicedVolumeProcessorPrivate<Voxel>;
    using Unit = typename Processor<Voxel>::Unit;
    using UnitDSFunc = typename DownSamplingOp<Voxel>::Func;
    using UnitMPFunc = typename MappingOp<Voxel>::Func;
    using UnitSSFunc = StatisticsOp<Voxel>;
    std::unique_ptr<RawGridVolumeReader> raw_reader;
    Unit src;
    std::unordered_map<VolumeType, std::queue<Unit>> unit_mp;
    int type_mask;
    VolumeRange range;

    void Init(){
        raw_reader = std::make_unique<RawGridVolumeReader>(src.desc_filename);
    }

    void Reset(){
        raw_reader.reset();
        unit_mp.clear();
        type_mask = 0;
        range = {0, 0, 0, 0, 0, 0};
    }

    template<VolumeType... types>
    void Convert();

    template<>
    void Convert<VolumeType::Grid_RAW>(){
        assert(unit_mp.count(VolumeType::Grid_RAW));
        auto& units = unit_mp[VolumeType::Grid_RAW];

        typename IOImpl<Voxel>::PackedParams packed = {
                .reader = raw_reader.get(),
                .range = range,
        };

        std::vector<std::unique_ptr<RawGridVolumeWriter>> raw_writers;
        while(!units.empty()){
            auto unit = units.front();
            units.pop();
            // init writer
            assert(unit.type == VolumeType::Grid_RAW);

            auto& writer = raw_writers.emplace_back(std::make_unique<RawGridVolumeWriter>(unit.desc_filename, unit.desc.raw_desc));

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

        typename IOImpl<Voxel>::PackedParams packed = {
                .reader = raw_reader.get(),
                .range = range,
        };
        std::vector<std::unique_ptr<SlicedGridVolumeWriter>> writers;
        while(!units.empty()){
            auto unit = units.front();
            units.pop();
            // init writer
            assert(unit.type == VolumeType::Grid_SLICED);

//            SlicedGridVolumeWriter writer(unit.desc_filename, unit.desc.sliced_desc);

            writers.push_back(std::make_unique<SlicedGridVolumeWriter>(
                    unit.desc_filename, unit.desc.sliced_desc
                    ));

            int op_mask = unit.ops.op_mask;
            bool has_mp = op_mask & Mapping;
            bool has_ds = op_mask & DownSampling;
            bool has_ss = op_mask & Statistics;

            auto ss = std::make_shared<StatisticsOp<Voxel>>();
            auto down_sampling_func = unit.ops.down_sampling.GetOp();
            auto mapping_func = unit.ops.mapping.GetOp();

            packed.writers.push_back({
                                             .other_writer = writers.back().get(),
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
    void Convert<VolumeType::Grid_RAW,VolumeType::Grid_SLICED>(){
        typename IOImpl<Voxel>::PackedParams packed = {
                .reader = raw_reader.get(),
                .range = range,
        };
        // raw
        std::vector<std::unique_ptr<RawGridVolumeWriter>> raw_writers;
        {
            auto &units = unit_mp[VolumeType::Grid_RAW];
            while (!units.empty()) {
                auto unit = units.front();
                units.pop();
                // init writer
                assert(unit.type == VolumeType::Grid_RAW);

                auto& writer = raw_writers.emplace_back(std::make_unique<RawGridVolumeWriter>(unit.desc_filename, unit.desc.raw_desc));

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
        }
        // slice
        std::vector<std::unique_ptr<SlicedGridVolumeWriter>> slice_writers;
        {
            auto& units = unit_mp[VolumeType::Grid_SLICED];
            while(!units.empty()){
                auto unit = units.front();
                units.pop();
                // init writer
                assert(unit.type == VolumeType::Grid_SLICED);

                auto& writer = slice_writers.emplace_back(std::make_unique<SlicedGridVolumeWriter>(unit.desc_filename, unit.desc.sliced_desc));

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
        }
        IOImpl<Voxel>::ConvertRawOrSlicedImpl(packed);
    }

    template<>
    void Convert<VolumeType::Grid_BLOCKED_ENCODED>(){
        auto oblocked_encoded_unit = unit_mp[VolumeType::Grid_BLOCKED_ENCODED].front();
        unit_mp[VolumeType::Grid_BLOCKED_ENCODED].pop();
        typename IOImpl<Voxel>::PackedParams packed = {
                .reader = raw_reader.get(),
                .range = range,
                .oblocked_encoded_unit = oblocked_encoded_unit
        };
        IOImpl<Voxel>::template ConvertBlockedEncodedImpl<false>(packed);
    }

    template<>
    void Convert<VolumeType::Grid_RAW,VolumeType::Grid_BLOCKED_ENCODED>(){
        if(unit_mp[VolumeType::Grid_BLOCKED_ENCODED].size() > 1){
            Convert<VolumeType::Grid_RAW>();
            Convert<VolumeType::Grid_BLOCKED_ENCODED>();
            return;
        }
        assert(unit_mp[VolumeType::Grid_BLOCKED_ENCODED].size() == 1);

        auto oblocked_encoded_unit = unit_mp[VolumeType::Grid_BLOCKED_ENCODED].front();
        unit_mp[VolumeType::Grid_BLOCKED_ENCODED].pop();

        std::vector<std::unique_ptr<RawGridVolumeWriter>> writers;
        typename IOImpl<Voxel>::PackedParams packed = {
                .reader = raw_reader.get(),
                .range = range,
                .oblocked_encoded_unit = oblocked_encoded_unit,
        };
        auto& units = unit_mp[VolumeType::Grid_RAW];
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
        IOImpl<Voxel>::template ConvertBlockedEncodedImpl<true>(packed);
    }

    template<>
    void Convert<VolumeType::Grid_SLICED,VolumeType::Grid_BLOCKED_ENCODED>(){
        if(unit_mp[VolumeType::Grid_BLOCKED_ENCODED].size() > 1
           || unit_mp[VolumeType::Grid_SLICED].size() > 1){
            Convert<VolumeType::Grid_SLICED>();
            Convert<VolumeType::Grid_BLOCKED_ENCODED>();
            return;
        }
        assert(unit_mp[VolumeType::Grid_SLICED].size() == 1 && unit_mp[VolumeType::Grid_BLOCKED_ENCODED].size() == 1);
#ifdef SLICE_FILE_NO_RANDOM_ACCESS
        Convert<VolumeType::Grid_SLICED>();
        Convert<VolumeType::Grid_BLOCKED_ENCODED>();
#else
        auto oslice_unit = unit_mp[VolumeType::Grid_SLICED].front();
        unit_mp[VolumeType::Grid_SLICED].pop();
        auto oblocked_encoded_unit = unit_mp[VolumeType::Grid_BLOCKED_ENCODED].front();
        unit_mp[VolumeType::Grid_BLOCKED_ENCODED].pop();
        auto oslice_desc = oslice_unit.desc.sliced_desc;
        auto other_ss = std::make_shared<StatisticsOp<Voxel>>();
        SlicedGridVolumeWriter other_writer(oslice_unit.desc_filename, oslice_desc);
        typename IOImpl<Voxel>::PackedParams0 packed{
                .reader = raw_reader.get(),
                .range = range,
                .oblocked_encoded_unit = oblocked_encoded_unit,
                .other_writer = &other_writer,
                .other_has_ds = static_cast<bool>(oslice_unit.ops.op_mask & DownSampling),
                .other_has_mp = static_cast<bool>(oslice_unit.ops.op_mask & Mapping),
                .other_has_ss = static_cast<bool>(oslice_unit.ops.op_mask & Statistics),
                .other_ds_func = oslice_unit.ops.down_sampling.GetOp(),
                .other_mp_func = oslice_unit.ops.mapping.GetOp(),
                .other_ss_func = other_ss
        };
        IOImpl<Voxel>::template ConvertBlockedEncodedImpl<true>(packed);
#endif
    }

    std::unordered_map<int, std::function<void()>> mask_func;

    RawVolumeProcessorPrivate(){
        mask_func[Grid_RAW_BIT] = [this]{ Convert<VolumeType::Grid_RAW>(); };
        mask_func[Grid_SLICED_BIT] = [this]{ Convert<VolumeType::Grid_SLICED>(); };
        mask_func[Grid_RAW_BIT | Grid_SLICED_BIT] = [this]{
            Convert<VolumeType::Grid_RAW, VolumeType::Grid_SLICED>();
        };
        mask_func[Grid_BLOCKED_ENCODED_BIT] = [this]{ Convert<VolumeType::Grid_BLOCKED_ENCODED>(); };
        mask_func[Grid_SLICED_BIT | Grid_BLOCKED_ENCODED_BIT] = [this]{
            Convert<VolumeType::Grid_SLICED, VolumeType::Grid_BLOCKED_ENCODED>();
        };
        mask_func[Grid_RAW_BIT | Grid_BLOCKED_ENCODED_BIT] = [this]{
            Convert<VolumeType::Grid_RAW, VolumeType::Grid_BLOCKED_ENCODED>();
        };
    }
};

template<typename Voxel>
VolumeProcessor<Voxel, VolumeType::Grid_RAW>::VolumeProcessor() {
    _ = std::make_unique<RawVolumeProcessorPrivate<Voxel>>();
}

template<typename Voxel>
VolumeProcessor<Voxel, VolumeType::Grid_RAW>::~VolumeProcessor() {

}

template<typename Voxel>
VolumeProcessor<Voxel, VolumeType::Grid_RAW>&
VolumeProcessor<Voxel, VolumeType::Grid_RAW>::SetSource(const Unit& u, const VolumeRange& range) {
    assert(u.type == VolumeType::Grid_RAW);
//    if(!CheckValidation(u.desc.raw_desc)){
//        std::cerr << "Invalid source volume desc" << std::endl;
//        return *this;
//    }
    _->Reset();
    _->src = u;
    ReadDescFromFile(_->src.desc.raw_desc, _->src.desc_filename);
    _->range = range;
    _->Init();
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
    _->unit_mp[u.type].push(u);

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
    _->type_mask = 0;
}

template class VolumeProcessor<VoxelRU8, vol::VolumeType::Grid_RAW>;
template class VolumeProcessor<VoxelRU16, vol::VolumeType::Grid_RAW>;
