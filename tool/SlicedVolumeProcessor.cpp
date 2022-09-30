#include "VolumeProcessor.hpp"
#include "Detail/IOImpl.hpp"
#include <iostream>

template<typename Voxel>
class SlicedVolumeProcessorPrivate{
public:
    using Self = SlicedVolumeProcessorPrivate<Voxel>;
    using Unit = typename Processor<Voxel>::Unit;
    using UnitDSFunc = typename DownSamplingOp<Voxel>::Func;
    using UnitMPFunc = typename MappingOp<Voxel>::Func;
    using UnitSSFunc = StatisticsOp<Voxel>;
    std::unique_ptr<SlicedGridVolumeReader> sliced_reader;
    Unit src;
    // process units of one VolumeType by sequence order
    std::unordered_map<VolumeType, std::queue<Unit>> unit_mp;
    int type_mask;
    VolumeRange range;


    size_t CalSliceSize() const{
        return GetVoxelSize(Voxel::type, Voxel::format) * (range.dst_x - range.src_x) * (range.dst_y - range.src_y);
    }


    void DownSamplingInplace(SliceDataView<Voxel>& slice_view, DownSamplingOp<Voxel>::Func& func){
        int h = slice_view.sizeY / 2;
        int w = slice_view.sizeX / 2;
        //todo using parallel to speed up
        for(int i = 0; i < h; i++){
            for(int j = 0; j < w; j++){
                slice_view.At(j, i) = func(slice_view.At(j * 2, i), slice_view.At(j * 2 + 1, i));
            }
            if(w * 2 < slice_view.sizeX)
                slice_view.At(w , i) = slice_view.At(w * 2, i);
        }
        w = (slice_view.sizeX + 1) / 2;
        for(int i = 0; i < h; i++){
            for(int j = 0; j < w; j++)
                slice_view.At(j, i) = func(slice_view.At(j, i * 2), slice_view.At(j, i * 2 + 1));
        }
        if(h * 2 < slice_view.sizeY){
            for(int j = 0; j < w; j++)
                slice_view.At(j, h) = slice_view.At(j, h * 2);
        }
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
                .reader = sliced_reader.get(),
                .range = range,
        };
        while(!units.empty()){
            auto unit = units.front();
            units.pop();
            // init writer
            assert(unit.type == VolumeType::Grid_RAW);

            RawGridVolumeWriter writer(unit.desc_filename, unit.desc.raw_desc);

            int op_mask = unit.ops.op_mask;
            bool has_mp = op_mask & Mapping;
            bool has_ds = op_mask & DownSampling;
            bool has_ss = op_mask & Statistics;

            auto ss = std::make_shared<StatisticsOp<Voxel>>();
            auto down_sampling_func = unit.ops.down_sampling.GetOp();
            auto mapping_func = unit.ops.mapping.GetOp();

            packed.writers.push_back({
                                             .other_writer = &writer,
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
                .reader = sliced_reader.get(),
                .range = range,
        };
        while(!units.empty()){
            auto unit = units.front();
            units.pop();
            // init writer
            assert(unit.type == VolumeType::Grid_SLICED);

            SlicedGridVolumeWriter writer(unit.desc_filename, unit.desc.sliced_desc);

            int op_mask = unit.ops.op_mask;
            bool has_mp = op_mask & Mapping;
            bool has_ds = op_mask & DownSampling;
            bool has_ss = op_mask & Statistics;

            auto ss = std::make_shared<StatisticsOp<Voxel>>();
            auto down_sampling_func = unit.ops.down_sampling.GetOp();
            auto mapping_func = unit.ops.mapping.GetOp();

            packed.writers.push_back({
                .other_writer = &writer,
                .other_has_ds = has_ds,
                .other_has_mp = has_mp,
                .other_has_ss = has_ss,
                .other_ds_func = down_sampling_func,
                .other_mp_func = mapping_func,
                .other_ss_func = ss
            });

            /*
            auto slice_bytes = CalSliceSize();
            std::vector<uint8_t> slice_buffer(slice_bytes);
            auto dummy_slice_buffer = slice_buffer;

            // read source slice in each iterate, maybe optimized later
            for(int z = range.src_z; z <= range.dst_z; z++){
                auto ptr = (z - range.src_z) % 2 == 0 ? slice_buffer.data() : dummy_slice_buffer.data();
                sliced_reader->ReadSliceData(z, range.src_x, range.src_y,range.dst_x, range.dst_y, ptr, slice_bytes);
                SliceDataView<Voxel> slice_view(range.dst_x - range.src_x,
                                                range.dst_y - range.src_y,reinterpret_cast<Voxel*>(ptr));
                if(has_mp){

                    for(int i = 0; i < slice_h; i++){
                        for(int j = 0; j < slice_w; j++){
                            auto& voxel = slice_view.At(j, i);
                            voxel = mapping_func(voxel);
                        }
                    }
                }
                if(has_ds){
                    if((z - range.src_z) % 2){
                        // down sampling
                        auto dummy_ptr = dummy_slice_buffer.data();
                        SliceDataView<Voxel> dummy_slice_view(range.dst_x - range.src_x,
                                                              range.dst_y - range.src_y,reinterpret_cast<Voxel*>(dummy_ptr));
                        for(int i = 0; i < slice_h; i++){
                            for(int j = 0; j < slice_w; j++){
                                slice_view.At(j, i) = down_sampling_func(slice_view.At(j, i), dummy_slice_view.At(j, i));
                            }
                        }
                    }
                }
                if(has_ss){
                   if(!has_ds || (z - range.src_z) % 2){
                       // no down sampling so statistics each z
                       // or has down sampling only statistics for odd z
                       for(int i = 0; i < slice_h; i++){
                           for(int j = 0; j < slice_w; j++){
                               ss->AddVoxel(slice_view.At(j, i));
                           }
                       }
                   }
                }
                if(!has_ds || (z - range.src_z) % 2){
                    // write slice
                    //to down sampling in slice
                    DownSamplingInplace(slice_view, down_sampling_func);
                    writer.WriteSliceData(z - range.src_z, ptr, slice_bytes);
                }

            }
            */
        }
        IOImpl<Voxel>::ConvertRawOrSlicedImpl(packed);
    }

    template<>
    void Convert<VolumeType::Grid_RAW,VolumeType::Grid_SLICED>(){
        int slice_w = range.dst_x - range.src_x;
        int slice_h = range.dst_y - range.src_y;

        typename IOImpl<Voxel>::PackedParams1 packed = {
                .reader = sliced_reader.get(),
                .range = range,
        };
        // raw
        {
            auto &units = unit_mp[VolumeType::Grid_RAW];
            while (!units.empty()) {
                auto unit = units.front();
                units.pop();
                // init writer
                assert(unit.type == VolumeType::Grid_RAW);

                RawGridVolumeWriter writer(unit.desc_filename, unit.desc.raw_desc);

                int op_mask = unit.ops.op_mask;
                bool has_mp = op_mask & Mapping;
                bool has_ds = op_mask & DownSampling;
                bool has_ss = op_mask & Statistics;

                auto ss = std::make_shared<StatisticsOp<Voxel>>();
                auto down_sampling_func = unit.ops.down_sampling.GetOp();
                auto mapping_func = unit.ops.mapping.GetOp();

                packed.writers.push_back({
                                                 .other_writer = &writer,
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
        {
            auto& units = unit_mp[VolumeType::Grid_SLICED];
            while(!units.empty()){
                auto unit = units.front();
                units.pop();
                // init writer
                assert(unit.type == VolumeType::Grid_SLICED);

                SlicedGridVolumeWriter writer(unit.desc_filename, unit.desc.sliced_desc);

                int op_mask = unit.ops.op_mask;
                bool has_mp = op_mask & Mapping;
                bool has_ds = op_mask & DownSampling;
                bool has_ss = op_mask & Statistics;

                auto ss = std::make_shared<StatisticsOp<Voxel>>();
                auto down_sampling_func = unit.ops.down_sampling.GetOp();
                auto mapping_func = unit.ops.mapping.GetOp();

                packed.writers.push_back({
                                                 .other_writer = &writer,
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
        typename IOImpl<Voxel>::PackedParams0 packed = {
                .reader = sliced_reader.get(),
                .range = range,
                .oblocked_encoded_unit = oblocked_encoded_unit
        };
        IOImpl<Voxel>::template ConvertBlockedEncodedImpl<false>({});
    }

    template<>
    void Convert<VolumeType::Grid_SLICED, VolumeType::Grid_BLOCKED_ENCODED>(){
        if(unit_mp[VolumeType::Grid_BLOCKED_ENCODED].size() > 1
        || unit_mp[VolumeType::Grid_SLICED].size() > 1){
            Convert<VolumeType::Grid_SLICED>();
            Convert<VolumeType::Grid_BLOCKED_ENCODED>();
            return;
        }
        assert(unit_mp[VolumeType::Grid_SLICED].size() == 1 && unit_mp[VolumeType::Grid_BLOCKED_ENCODED].size() == 1);

        auto oslice_unit = unit_mp[VolumeType::Grid_SLICED].front();
        unit_mp[VolumeType::Grid_SLICED].pop();
        auto oblocked_encoded_unit = unit_mp[VolumeType::Grid_BLOCKED_ENCODED].front();
        unit_mp[VolumeType::Grid_BLOCKED_ENCODED].pop();
        auto oslice_desc = oslice_unit.desc.sliced_desc;
        auto other_ss = std::make_shared<StatisticsOp<Voxel>>();
        SlicedGridVolumeWriter other_writer(oslice_unit.desc_filename, oslice_desc);
        typename IOImpl<Voxel>::PackedParams0 packed{
                .reader = sliced_reader.get(),
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
        IOImpl<Voxel>::template ConvertBlockedEncodedImpl<true>({});
        /*
        auto& oslice_unit = unit_mp[VolumeType::Grid_SLICED].front();
        auto oslice_desc = oslice_unit.desc.sliced_desc;
        auto& oblocked_encoded_unit = unit_mp[VolumeType::Grid_BLOCKED_ENCODED].front();
        {
            // down sampling for encoded blocked volume will not have impact
            if(oblocked_encoded_unit.ops.op_mask & DownSampling){
                std::cerr << "down sampling for encoded blocked volume will not have impact" << std::endl;
            }
        }

        auto& oblocked_encoded_desc = oblocked_encoded_unit.desc.encoded_blocked_desc;
        uint32_t block_length = oblocked_encoded_desc.block_length;
        assert(block_length % 2 == 0);
        uint32_t padding = oblocked_encoded_desc.padding;
        uint32_t block_size = block_length + 2 * padding;
        uint32_t read_x_size = range.dst_x - range.src_x;
        uint32_t read_y_size = range.dst_y - range.src_y;
        uint32_t read_z_size = range.dst_z - range.src_z;
        Extend3D grid_extend{read_x_size, block_size, block_size};
        RawGridVolume<Voxel> grid(RawGridVolumeDesc{.extend = grid_extend});
        RawGridVolume<Voxel> block(RawGridVolumeDesc{.extend = {block_size, block_size, block_size}});
        // note read pos base on block_length but read region should base on block_size
        int y_read_count = (read_y_size + block_length - 1) / block_length;
        int z_read_count = (read_z_size + block_length - 1) / block_length;
        int x_block_count = (read_x_size + block_length - 1) / block_length;

        EncodedBlockedGridVolumeWriter encoded_blocked_writer(oblocked_encoded_unit.output, oblocked_encoded_desc);
        SlicedGridVolumeWriter sliced_writer(oslice_unit.output, oslice_desc);

        bool slice_has_ds = oslice_unit.ops.op_mask & DownSampling;
        bool slice_has_mp = oslice_unit.ops.op_mask & Mapping;
        bool slice_has_ss = oslice_unit.ops.op_mask & Statistics;
        auto slice_mapping_func = oslice_unit.ops.mapping.GetOp();
        auto slice_ds_func = oslice_unit.ops.down_sampling.GetOp();
        StatisticsOp<Voxel> slice_ss;
        auto eb_mapping_func = oblocked_encoded_unit.ops.mapping.GetOp();
        StatisticsOp<Voxel> eb_ss;
        for(int y_turn = 0; y_turn < y_read_count; y_turn++){
            for(int z_turn = 0; z_turn < z_read_count; z_turn++){
                sliced_reader->ReadVolumeData(range.src_x - padding,
                                              range.src_y + y_turn * block_length - padding,
                                              range.src_z + z_turn * block_length - padding,
                                              range.dst_x + padding,
                                              range.dst_y + (y_turn + 1) * block_length + padding,
                                              range.dst_z + (z_turn + 1) * block_length + padding,
                                              grid.GetRawDataPtr(), grid_extend.size() * sizeof(Voxel));
                // write grid's blocks into file
                for(int x_turn = 0; x_turn < x_block_count; x_turn++){
                    encoded_blocked_writer.WriteBlockData({x_turn, y_turn, z_turn},
                                                          [&](int dx, int dy, int dz, void* dst, size_t ele_size){
                        auto p = reinterpret_cast<Voxel*>(dst);
                        *p = eb_mapping_func(grid(x_turn * block_length - padding + dx,
                                  y_turn * block_length - padding + dy,
                                  z_turn * block_length - padding + dz));
                        eb_ss.AddVoxel(*p);
                    });
                }
                // write grid to slices
                if(!slice_has_ds){
                    sliced_writer.WriteVolumeData(0, y_turn * block_length, z_turn * block_length,
                                                  read_x_size, (y_turn + 1) * block_length, (z_turn + 1) * block_length,
                                                  [&](int dx, int dy, int dz, void* dst, size_t ele_size){
                        auto p = reinterpret_cast<Voxel*>(dst);
                        *p = slice_mapping_func(grid(dx, y_turn * block_length + dy, z_turn * block_length + dz));
                        slice_ss.AddVoxel(*p);
                    });
                }
                else{
                    sliced_writer.WriteVolumeData(0, y_turn * block_length / 2, z_turn * block_length / 2,
                                                  (read_x_size + 1) / 2,// x y z out of slice region will be handled by WriteVolumeData
                                                  (y_turn + 1) * block_length / 2,
                                                  (z_turn + 1) * block_length / 2,
                                                  [&, beg_x = 0, beg_y = y_turn * block_length / 2, beg_z = z_turn * block_length / 2]
                                                  (int dx, int dy, int dz, void* dst, size_t ele_size){
                        auto p = reinterpret_cast<Voxel*>(dst);
                        int x = 2 * (beg_x + dx), y = 2 * (beg_y + dy), z = 2 * (beg_z + dz);
                        *p = slice_mapping_func(slice_ds_func(
                        slice_ds_func(slice_ds_func(grid(x, y, z), grid(x + 1, y, z)),
                        slice_ds_func(grid(x, y + 1, z), grid(x + 1, y + 1, z))),
                        slice_ds_func(slice_ds_func(grid(x, y, z + 1), grid(x + 1, y, z + 1)),
                        slice_ds_func(grid(x, y + 1, z + 1), grid(x + 1, y + 1, z + 1)))));
                        slice_ss.AddVoxel(*p);
                    });
                }
            }
        }
        */
    }

    template<>
    void Convert<VolumeType::Grid_RAW, VolumeType::Grid_BLOCKED_ENCODED>(){
        if(unit_mp[VolumeType::Grid_BLOCKED_ENCODED].size() > 1
           || unit_mp[VolumeType::Grid_RAW].size() > 1){
            Convert<VolumeType::Grid_RAW>();
            Convert<VolumeType::Grid_BLOCKED_ENCODED>();
            return;
        }
        assert(unit_mp[VolumeType::Grid_RAW].size() == 1 && unit_mp[VolumeType::Grid_BLOCKED_ENCODED].size() == 1);

        auto oraw_unit = unit_mp[VolumeType::Grid_RAW].front();
        unit_mp[VolumeType::Grid_RAW].pop();
        auto oblocked_encoded_unit = unit_mp[VolumeType::Grid_BLOCKED_ENCODED].front();
        unit_mp[VolumeType::Grid_BLOCKED_ENCODED].pop();
        auto oraw_desc = oraw_unit.desc.sliced_desc;
        auto other_ss = std::make_shared<StatisticsOp<Voxel>>();
        RawGridVolumeWriter other_writer(oraw_unit.desc_filename, oraw_desc);
        typename IOImpl<Voxel>::PackedParams0 packed = {
                .reader = sliced_reader.get(),
                .range = range,
                .oblocked_encoded_unit = oblocked_encoded_unit,
                .other_writer = &other_writer,
                .other_has_ds = static_cast<bool>(oraw_unit.ops.op_mask & DownSampling),
                .other_has_mp = static_cast<bool>(oraw_unit.ops.op_mask & Mapping),
                .other_has_ss = static_cast<bool>(oraw_unit.ops.op_mask & Statistics),
                .other_ds_func = oraw_unit.ops.down_sampling.GetOp(),
                .other_mp_func = oraw_unit.ops.mapping.GetOp(),
                .other_ss_func = other_ss
        };
        IOImpl<Voxel>::template ConvertBlockedEncodedImpl<true>({});

    }

    std::unordered_map<int, std::function<void()>> mask_func;

    SlicedVolumeProcessorPrivate(){
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
    _->unit_mp[u.type].push(u);

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
template class VolumeProcessor<VoxelRU16, vol::VolumeType::Grid_SLICED>;