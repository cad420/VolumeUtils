#pragma once

#include "VolumeProcessor.hpp"

template<typename Voxel>
struct IOImpl {
    using Unit = typename Processor<Voxel>::Unit;
    using UnitDSFunc = typename DownSamplingOp<Voxel>::Func;
    using UnitMPFunc = typename MappingOp<Voxel>::Func;
    using UnitSSFunc = StatisticsOp<Voxel>;

    struct PackedParams0 {
        CVolumeReaderInterface *reader = nullptr;
        VolumeRange range;
        Unit oblocked_encoded_unit;
        CVolumeWriterInterface *other_writer = nullptr;
        bool other_has_ds = false;
        bool other_has_mp = false;
        bool other_has_ss = false;
        UnitDSFunc other_ds_func;
        UnitMPFunc other_mp_func;
        std::shared_ptr<UnitSSFunc> other_ss_func;
    };

    template<bool Other>
    static void ConvertBlockedEncodedImpl(const PackedParams0 &params) {
        // structure binding can be explicitly captured by lambda since c++20
        auto [reader, range, oblocked_encoded_unit, other_writer,
                other_has_ds, other_has_mp, other_has_ss,
                other_ds_func, other_mp_func, other_ss_func] = params;

        // init blocked info
        auto &oblocked_encoded_desc = oblocked_encoded_unit.desc.encoded_blocked_desc;
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
        auto eb_mapping_func = oblocked_encoded_unit.ops.mapping.GetOp();
        StatisticsOp<Voxel> eb_ss;

        for (int y_turn = 0; y_turn < y_read_count; y_turn++) {
            for (int z_turn = 0; z_turn < z_read_count; z_turn++) {
                reader->ReadVolumeData(range.src_x - padding,
                                       range.src_y + y_turn * block_length - padding,
                                       range.src_z + z_turn * block_length - padding,
                                       range.dst_x + padding,
                                       range.dst_y + (y_turn + 1) * block_length + padding,
                                       range.dst_z + (z_turn + 1) * block_length + padding,
                                       grid.GetRawDataPtr(), grid_extend.size() * sizeof(Voxel));
                // write grid's blocks into file
                for (int x_turn = 0; x_turn < x_block_count; x_turn++) {
                    encoded_blocked_writer.WriteBlockData({x_turn, y_turn, z_turn},
                                                          [&](int dx, int dy, int dz, void *dst, size_t ele_size) {
                                                              auto p = reinterpret_cast<Voxel *>(dst);
                                                              *p = eb_mapping_func(
                                                                      grid(x_turn * block_length - padding + dx,
                                                                           y_turn * block_length - padding + dy,
                                                                           z_turn * block_length - padding + dz));
                                                              eb_ss.AddVoxel(*p);
                                                          });
                }
                if (!Other) continue;
                // write grid to slices
                if (!other_has_ds) {
                    other_writer->WriteVolumeData(0, y_turn * block_length, z_turn * block_length,
                                                  read_x_size, (y_turn + 1) * block_length, (z_turn + 1) * block_length,
                                                  [&, other_mp_func = other_mp_func, other_ss_func = other_ss_func]
                                                          (int dx, int dy, int dz, void *dst, size_t ele_size) {
                                                      auto p = reinterpret_cast<Voxel *>(dst);
                                                      *p = other_mp_func(grid(dx, y_turn * block_length + dy,
                                                                              z_turn * block_length + dz));
                                                      other_ss_func->AddVoxel(*p);
                                                  });
                } else {
                    other_writer->WriteVolumeData(0, y_turn * block_length / 2, z_turn * block_length / 2,
                                                  (read_x_size + 1) /
                                                  2,// x y z out of slice region will be handled by WriteVolumeData
                                                  (y_turn + 1) * block_length / 2,
                                                  (z_turn + 1) * block_length / 2,
                                                  [&, beg_x = 0, end_x = read_x_size,
                                                          beg_y = y_turn * block_length / 2, beg_z =
                                                  z_turn * block_length / 2,
                                                          other_mp_func = other_mp_func, other_ds_func = other_ds_func,
                                                          other_ss_func = other_ss_func]
                                                          (int dx, int dy, int dz, void *dst, size_t ele_size) {
                                                      auto p = reinterpret_cast<Voxel *>(dst);
                                                      // x read size maybe odd while y and z read size must be even
                                                      int x = 2 * (beg_x + dx), y = 2 * (beg_y + dy), z =
                                                              2 * (beg_z + dz);
                                                      int x1 = std::min<int>(end_x - 1, x + 1);
                                                      *p = other_mp_func(other_ds_func(
                                                              other_ds_func(
                                                                      other_ds_func(grid(x, y, z), grid(x1, y, z)),
                                                                      other_ds_func(grid(x, y + 1, z),
                                                                                    grid(x1, y + 1, z))),
                                                              other_ds_func(other_ds_func(grid(x, y, z + 1),
                                                                                          grid(x1, y, z + 1)),
                                                                            other_ds_func(grid(x, y + 1, z + 1),
                                                                                          grid(x1, y + 1, z + 1)))));

                                                      other_ss_func->AddVoxel(*p);
                                                  });
                }
            }
        }
    }

    struct WriterParams {
        CVolumeWriterInterface *other_writer = nullptr;
        bool other_has_ds = false;
        bool other_has_mp = false;
        bool other_has_ss = false;
        UnitDSFunc other_ds_func;
        UnitMPFunc other_mp_func;
        std::shared_ptr<UnitSSFunc> other_ss_func;
    };

    struct PackedParams1 {
        CVolumeReaderInterface *reader;
        VolumeRange range;
        std::vector<WriterParams> writers;
    };


    static size_t CalSliceSize(const VolumeRange &range) {
        return GetVoxelSize(Voxel::type, Voxel::format) * (range.dst_x - range.src_x) * (range.dst_y - range.src_y);
    }

    static void ConvertRawOrSlicedImpl(const PackedParams1 &params) {
        // read two slice of volume
        auto reader = params.reader;
        auto range = params.range;
        auto slice_bytes = CalSliceSize(range);
        int slice_w = range.dst_x - range.src_x;
        int slice_h = range.dst_y - range.src_y;
        int slice_count = range.dst_z - range.src_z;
        Extend3D extend = {(uint32_t)slice_w, (uint32_t)slice_h, 2u};
        RawGridVolume<Voxel> grid(RawGridVolumeDesc{.extend = extend});

        for (int z = range.src_z; z < range.dst_z; z++) {
            bool is_even = (z - range.src_z) % 2;
            auto slice_buffer_ptr = reinterpret_cast<Voxel *>(grid.GetRawDataPtr() + slice_w * slice_h * int(is_even));
            reader->ReadVolumeData(range.src_x, range.src_y, range.src_z, range.dst_x, range.dst_y, range.dst_z,
                                   slice_buffer_ptr, slice_bytes);

            if (!is_even) continue;

            for (auto &param: params.writers) {
                if (!param.other_has_ds) {
                    param.other_writer->WriteVolumeData(0, 0, z - range.src_z, slice_w, slice_h, z - range.src_z + 2,
                                                        [&, other_mp_func = param.other_mp_func,
                                                                other_ss_func = param.other_ss_func]
                                                                (int dx, int dy, int dz, void *dst, size_t ele_size) {
                                                            auto p = reinterpret_cast<Voxel *>(dst);
                                                            *p = other_mp_func(grid(dx, dy, dz));
                                                            other_ss_func->AddVoxel(*p);
                                                        });
                } else {
                    param.other_writer->WriteVolumeData(0, 0, (z - range.src_z) / 2, slice_w / 2, slice_h / 2,
                                                        z - range.src_z + 1,
                                                        [&, other_mp_func = param.other_mp_func,
                                                                other_ss_func = param.other_ss_func, other_ds_func = param.other_ds_func,
                                                                end_x = slice_w, end_y = slice_h, end_z = slice_count]
                                                                (int dx, int dy, int dz, void *dst, size_t ele_size) {
                                                            auto p = reinterpret_cast<Voxel *>(dst);
                                                            int x = 2 * dx, y = 2 * dy, z = 2 * dz;
                                                            int x1 = std::min(x + 1, end_x - 1);
                                                            int y1 = std::min(y + 1, end_y - 1);
                                                            int z1 = std::min(z + 1, end_z - 1);
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