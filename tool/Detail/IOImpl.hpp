#pragma once
#include <fstream>
#include "VolumeProcessor.hpp"
#include "Utils.hpp"
template<typename Voxel>
struct IOImpl {
    using Unit = typename Processor<Voxel>::Unit;
    using UnitDSFunc = typename DownSamplingOp<Voxel>::Func;
    using UnitMPFunc = typename MappingOp<Voxel>::Func;
    using UnitSSFunc = StatisticsOp<Voxel>;
    struct WriterParams {
        CVolumeWriterInterface *other_writer = nullptr;
        bool other_has_ds = false;
        bool other_has_mp = false;
        bool other_has_ss = false;
        UnitDSFunc other_ds_func;
        UnitMPFunc other_mp_func;
        std::shared_ptr<UnitSSFunc> other_ss_func;
    };
    struct PackedParams {
        CVolumeReaderInterface *reader = nullptr;
        VolumeRange range;
        Unit oblocked_encoded_unit;
        std::vector<WriterParams> writers;
    };



    template<bool Other, bool WriteEB = true>
    static void ConvertBlockedEncodedImpl(const PackedParams &params) {
        // structure binding can be explicitly captured by lambda since c++20
//        auto [reader, range, oblocked_encoded_unit, other_writer,
//                other_has_ds, other_has_mp, other_has_ss,
//                other_ds_func, other_mp_func, other_ss_func] = params;
        auto reader = params.reader;
        auto range = params.range;
        auto oblocked_encoded_unit = params.oblocked_encoded_unit;

        // init blocked info
        auto &oblocked_encoded_desc = oblocked_encoded_unit.desc.encoded_blocked_desc;
        const int block_length = oblocked_encoded_desc.block_length;
        assert(block_length % 2 == 0);
        const int padding = oblocked_encoded_desc.padding;
        const int block_size = block_length + 2 * padding;
        const int read_x_size = range.dst_x - range.src_x;
        const int read_y_size = range.dst_y - range.src_y;
        const int read_z_size = range.dst_z - range.src_z;
        // note read pos base on block_length but read region should base on block_size
        int y_read_count = (read_y_size + block_length - 1) / block_length;
        int z_read_count = (read_z_size + block_length - 1) / block_length;
        int x_block_count = (read_x_size + block_length - 1) / block_length;

        Extend3D grid_extend{(uint32_t)x_block_count * block_length + 2 * padding, (uint32_t)block_size, (uint32_t)block_size};
        VoxelInfo _voxel_info{.type = Voxel::type, .format = Voxel::format};
        RawGridVolumeDesc raw_desc{.extend = grid_extend}; raw_desc.voxel_info = _voxel_info;
        RawGridVolume<Voxel> grid(raw_desc);
        raw_desc.extend = {(uint32_t)block_size, (uint32_t)block_size, (uint32_t)block_size};
        RawGridVolume<Voxel> block(raw_desc);


        std::unique_ptr<EncodedBlockedGridVolumeWriter> encoded_blocked_writer;
        if(WriteEB)
            encoded_blocked_writer = std::make_unique<EncodedBlockedGridVolumeWriter>(oblocked_encoded_unit.desc_filename, oblocked_encoded_desc);
        bool eb_has_mp = oblocked_encoded_unit.ops.op_mask & Mapping;
        bool eb_has_ss = oblocked_encoded_unit.ops.op_mask & Statistics;
        auto eb_mapping_func = oblocked_encoded_unit.ops.mapping.GetOp();

        StatisticsOp<Voxel> eb_ss;
        progress_bar_t pb(30);
        for (int z_turn = 0; z_turn < z_read_count; z_turn++) {
            for (int y_turn = 0; y_turn < y_read_count; y_turn++) {
                reader->ReadVolumeData(range.src_x - padding,
                                       range.src_y + y_turn * block_length - padding,
                                       range.src_z + z_turn * block_length - padding,
                                       range.src_x + x_block_count * block_length + padding,
                                       range.src_y + (y_turn + 1) * block_length + padding,
                                       range.src_z + (z_turn + 1) * block_length + padding,
                                       grid.GetRawDataPtr());
                pb.set_percent(100.0 * (z_turn * y_read_count + y_turn) / (z_read_count * y_read_count));
                pb.display();
//                if(y_turn == 2 && z_turn == 1){
//                    std::ofstream out("H:/Volume/grid_1272_256_256_uint8.raw", std::ios::binary);
//                    out.write(reinterpret_cast<const char*>(grid.GetRawDataPtr()),
//                              grid_extend.size());
//                    out.close();
//                    std::cout << "write grid" << std::endl;
//                }
//                VOL_WHEN_DEBUG({
//                    std::vector<uint8_t> table(256, 0);
//                    auto p = reinterpret_cast<uint8_t*>(grid.GetRawDataPtr());
//                    for(int i = 0; i < grid_extend.size(); i++)
//                        table[p[i]]++;
//                    for(int i = 0; i < 256; i++)
//                        std::cout << "(" << i << " : " << (int)(table[i]) << ")" << std::endl;
//                })
                // write grid's blocks into file
                if(WriteEB){
                    for (int x_turn = 0; x_turn < x_block_count; x_turn++) {
                        VOL_WHEN_DEBUG(
                                std::cout << "write block : (" << x_turn << ", " << y_turn << ", " << z_turn << ")"
                                          << std::endl)
                        if (eb_has_mp)
                            encoded_blocked_writer->WriteBlockData({x_turn, y_turn, z_turn},
                                                                  [&](int dx, int dy, int dz, void *dst,
                                                                      size_t ele_size) {
                                                                      auto p = reinterpret_cast<Voxel *>(dst);
                                                                      int x = x_turn * block_length + dx;
                                                                      *p = eb_mapping_func(grid(x, dy, dz));
                                                                      eb_ss.AddVoxel(*p);
                                                                  });
                        else
                            encoded_blocked_writer->WriteBlockData({x_turn, y_turn, z_turn},
                                                                  [&](int dx, int dy, int dz, void *dst,
                                                                      size_t ele_size) {
                                                                      auto p = reinterpret_cast<Voxel *>(dst);
                                                                      int x = x_turn * block_length + dx;
                                                                      *p = grid(x, dy, dz);
                                                                  });
                    }

                }
                if (!Other) continue;
                // write grid to slices
                for(auto& param : params.writers){
                    if (!param.other_has_ds) {
                        param.other_writer->WriteVolumeData(-padding, y_turn * block_length - padding,
                                                      z_turn * block_length - padding,
                                                      x_block_count * block_length + padding,
                                                      (y_turn + 1) * block_length + padding,
                                                      (z_turn + 1) * block_length + padding,
                                                      [&, other_mp_func = param.other_mp_func, other_ss_func = param.other_ss_func]
                                                              (int dx, int dy, int dz, void *dst, size_t ele_size) {
                                                          auto p = reinterpret_cast<Voxel *>(dst);
                                                          *p = other_mp_func(grid(dx, dy, dz));
                                                          other_ss_func->AddVoxel(*p);
                                                      });
                    } else {
                        param.other_writer->WriteVolumeData(0, y_turn * block_length / 2, z_turn * block_length / 2,
                                                      (x_block_count * block_length) / 2,
                                                      (y_turn + 1) * block_length /
                                                      2,// divided must be even, so this is ok
                                                      (z_turn + 1) * block_length / 2,
                                                      [&, other_mp_func = param.other_mp_func, other_ds_func = param.other_ds_func,
                                                              other_ss_func = param.other_ss_func, padding = padding]
                                                              (int dx, int dy, int dz, void *dst, size_t ele_size) {
                                                          auto p = reinterpret_cast<Voxel *>(dst);
                                                          // x read size maybe odd while y and z read size must be even
                                                          int x = 2 * dx + padding, y = 2 * dy + padding, z =
                                                                  2 * dz + padding;
                                                          *p = other_mp_func(other_ds_func(
                                                                  other_ds_func(
                                                                          other_ds_func(grid(x, y, z),
                                                                                        grid(x + 1, y, z)),
                                                                          other_ds_func(grid(x, y + 1, z),
                                                                                        grid(x + 1, y + 1, z))),
                                                                  other_ds_func(other_ds_func(grid(x, y, z + 1),
                                                                                              grid(x + 1, y, z + 1)),
                                                                                other_ds_func(grid(x, y + 1, z + 1),
                                                                                              grid(x + 1, y + 1,
                                                                                                   z + 1)))));

                                                          other_ss_func->AddVoxel(*p);
                                                      });
                    }
                }
            }
        }
        pb.done();
    }




    static size_t CalSliceSize(const VolumeRange &range) {
        return GetVoxelSize(Voxel::type, Voxel::format) * (range.dst_x - range.src_x) * (range.dst_y - range.src_y);
    }

    static void ConvertRawOrSlicedImpl(const PackedParams &params) {
        // read two slice of volume
        auto reader = params.reader;
        auto range = params.range;
        auto slice_bytes = CalSliceSize(range);
        int slice_w = range.dst_x - range.src_x;
        int slice_h = range.dst_y - range.src_y;
        int slice_count = range.dst_z - range.src_z;
        Extend3D extend = {(uint32_t)slice_w, (uint32_t)slice_h, 2u};
        RawGridVolumeDesc raw_desc{.extend = extend};
        raw_desc.voxel_info = {.type = Voxel::type, .format = Voxel::format};
        RawGridVolume<Voxel> grid(raw_desc);

        for (int z = range.src_z; z < range.dst_z; z++) {
            bool is_even = (z - range.src_z) % 2;
            auto slice_buffer_ptr = reinterpret_cast<Voxel *>(grid.GetRawDataPtr() + slice_w * slice_h * int(is_even));
            reader->ReadVolumeData(range.src_x, range.src_y, z, range.dst_x, range.dst_y, z + 1,
                                   slice_buffer_ptr);
            bool is_last = z == range.dst_z - 1;
            if (!is_even && !is_last) continue;


            for (auto &param: params.writers) {
                if (!param.other_has_ds) {
                    if(param.other_has_mp)
                        param.other_writer->WriteVolumeData(0, 0, z - range.src_z - 1 + (!is_even), slice_w, slice_h, z - range.src_z + 1,
                                                        [&, other_mp_func = param.other_mp_func,
                                                                other_ss_func = param.other_ss_func]
                                                                (int dx, int dy, int dz, void *dst, size_t ele_size) {
                                                            auto p = reinterpret_cast<Voxel *>(dst);
                                                            *p = other_mp_func(grid(dx, dy, dz));
                                                            other_ss_func->AddVoxel(*p);
                                                        });
                    else
                        param.other_writer->WriteVolumeData(0, 0, z - range.src_z - 1 + (!is_even), slice_w, slice_h, z - range.src_z + 1,
                                                            [&, other_mp_func = param.other_mp_func,
                                                                    other_ss_func = param.other_ss_func]
                                                                    (int dx, int dy, int dz, void *dst, size_t ele_size) {
                                                                auto p = reinterpret_cast<Voxel *>(dst);
                                                                *p = grid(dx, dy, dz);
                                                                other_ss_func->AddVoxel(*p);
                                                            });
                } else {
                    param.other_writer->WriteVolumeData(0, 0, (z - range.src_z - is_even) / 2, slice_w / 2 + 1, slice_h / 2 + 1,
                                                        (z - range.src_z - is_even) / 2 + 1,
                                                        [&, other_mp_func = param.other_mp_func,
                                                                other_ss_func = param.other_ss_func, other_ds_func = param.other_ds_func,
                                                                end_x = slice_w, end_y = slice_h, end_z = slice_count]
                                                                (int dx, int dy, int dz, void *dst, size_t ele_size) {
                                                            auto p = reinterpret_cast<Voxel *>(dst);
                                                            int x = 2 * dx, y = 2 * dy, z = 2 * dz;
                                                            int x1 = std::min(x + 1, end_x - 1);
                                                            int y1 = std::min(y + 1, end_y - 1);
                                                            int z1 = std::min(z + 1, end_z - 1);
//                                                            auto v = grid(x, y, z);
                                                            *p = other_mp_func(other_ds_func(
                                                                    other_ds_func(other_ds_func(grid(x, y, z),
                                                                                                grid(x1, y, z)),
                                                                                  other_ds_func(grid(x, y1, z),
                                                                                                grid(x1, y1, z))),
                                                                    other_ds_func(other_ds_func(grid(x, y, z1),
                                                                                                grid(x1, y, z1)),
                                                                                  other_ds_func(grid(x, y1, z1),
                                                                                                grid(x1, y1, z1)))));
                                                            other_ss_func->AddVoxel(*p);
                                                        });
                }
            }
        }

    }

};