//
// Created by wyz on 2022/4/29.
//
#include <VolumeUtils/Volume.hpp>
#include <iostream>
#include <fstream>
#include <unordered_set>
using namespace vol;


void run(std::vector<std::string> raw_filenames, std::vector<std::string> eb_filenames){
    assert(raw_filenames.size() == eb_filenames.size());
    int n = raw_filenames.size();
    struct Reader{
        std::unique_ptr<EncodedBlockedGridVolumeReader> eb_reader;
        std::unique_ptr<RawGridVolumeReader> raw_reader;
        EncodedBlockedGridVolumeDesc desc;
        Extend3D blocked_dim;
    };
    std::unordered_map<int, Reader> mp;

    int m = 1000;
    int block_length = 0;
    int padding = 0;

    for(int i = 0; i < n; i++){
        auto& reader = mp[i];
        reader.eb_reader = std::make_unique<EncodedBlockedGridVolumeReader>(eb_filenames[i]);
        reader.raw_reader = std::make_unique<RawGridVolumeReader>(raw_filenames[i]);
        reader.desc = reader.eb_reader->GetVolumeDesc();
        if(i == 0){
            block_length = reader.desc.block_length;
            padding = reader.desc.padding;
        }
        reader.blocked_dim.width = (reader.desc.extend.width + block_length - 1) / block_length;
        reader.blocked_dim.height = (reader.desc.extend.height + block_length - 1) / block_length;
        reader.blocked_dim.depth = (reader.desc.extend.depth + block_length - 1) / block_length;
    }

    assert(block_length && padding);


    std::unordered_map<int,std::unordered_set<BlockIndex>> used_st;

    auto gen_idx = [&]()->BlockIndex{
        return BlockIndex{std::rand() % 100, std::rand() % 100, std::rand() % 100};
    };

    auto gen_lod = [&]()->int{
        return std::rand() & n;
    };

    auto calc = [&](const std::vector<uint8_t>& a, const std::vector<uint8_t>& b){
        assert(a.size() == b.size());
        size_t voxel_count = a.size();
        double mse = 0.0;
        for(int i = 0; i < voxel_count; i++){
            int x = a[i], y = b[i];
            mse += (x - y) * (x - y);
        }
        mse = mse / double(voxel_count) ;
        double ret = 20.0 * std::log10(255) - 10.0 * std::log10(mse);
        return ret;
    };

    double ret = 0.0;
    double inv_cnt = 1.0 / m;
    std::vector<uint8_t> buffer_a, buffer_b;
    {
        size_t bs = block_length + 2 * padding;
        bs *= bs * bs;
        buffer_a.resize(bs, 0);
        buffer_b.resize(bs, 0);
    }
    std::srand(std::time(NULL));
    for(int i = 0; i < m; i++){
        std::cerr << "start iteration : " << i << std::endl;

        while(true){
            auto lod = gen_lod();
            if(lod >= 0 && lod < n){
                auto idx = gen_idx();

                if(used_st[lod].count(idx)) continue;

                auto& reader = mp.at(lod);
                if(idx.x >= 0 && idx.x < reader.blocked_dim.width - 1
                && idx.y >= 0 && idx.y < reader.blocked_dim.height - 1
                && idx.z >= 0 && idx.z < reader.blocked_dim.depth - 1){
                    std::cerr<< "\t block index: " << idx.x << " " << idx.y << " " << idx.z << std::endl;
                    reader.eb_reader->ReadBlockData(idx, buffer_a.data());
                    int beg_x = idx.x * block_length - padding;
                    int beg_y = idx.y * block_length - padding;
                    int beg_z = idx.z * block_length - padding;
                    int end_x = idx.x * block_length + padding;
                    int end_y = idx.y * block_length + padding;
                    int end_z = idx.z * block_length + padding;
                    reader.raw_reader->ReadVolumeData(beg_x, beg_y, beg_z,
                                                      end_x, end_y, end_z, buffer_b.data());
                    auto r = calc(buffer_a, buffer_b);
                    if(std::isinf(r)) continue;
                    ret += inv_cnt * r;
                    used_st[lod].insert(idx);
                    break;
                }

            }
        }

    }

    std::cerr << "result PSNR : " << ret << std::endl;



}


int main(){

    std::vector<std::string> eb_filenames_256 = {
            "G:/VolMouseData/config/gen_mouse_lod0.encoded_blocked.desc.json",
            "G:/VolMouseData/config/gen_mouse_lod1.encoded_blocked.desc.json",
            "G:/VolMouseData/config/gen_mouse_lod2.encoded_blocked.desc.json",
            "G:/VolMouseData/config/gen_mouse_lod3.encoded_blocked.desc.json",
            "G:/VolMouseData/config/gen_mouse_lod4.encoded_blocked.desc.json",
            "G:/VolMouseData/config/gen_mouse_lod5.encoded_blocked.desc.json",
            "G:/VolMouseData/config/gen_mouse_lod6.encoded_blocked.desc.json"
    };


    std::vector<std::string> eb_filenames_512 = {
            "G:/VolMouseData/config/gen_mouse_lod0_9p1.encoded_blocked.desc.json",
            "G:/VolMouseData/config/gen_mouse_lod1_9p1.encoded_blocked.desc.json",
            "G:/VolMouseData/config/gen_mouse_lod2_9p1.encoded_blocked.desc.json",
            "G:/VolMouseData/config/gen_mouse_lod3_9p1.encoded_blocked.desc.json",
            "G:/VolMouseData/config/gen_mouse_lod4_9p1.encoded_blocked.desc.json",
            "G:/VolMouseData/config/gen_mouse_lod5_9p1.encoded_blocked.desc.json",
            "G:/VolMouseData/config/gen_mouse_lod6_9p1.encoded_blocked.desc.json"
    };

    std::vector<std::string> raw_filenames = {
            "G:/VolMouseData/config/mouse_lod0.raw.desc.json",
            "G:/VolMouseData/config/mouse_lod1.raw.desc.json",
            "G:/VolMouseData/config/mouse_lod2.raw.desc.json",
            "G:/VolMouseData/config/mouse_lod3.raw.desc.json",
            "G:/VolMouseData/config/mouse_lod4.raw.desc.json",
            "G:/VolMouseData/config/mouse_lod5.raw.desc.json",
            "G:/VolMouseData/config/mouse_lod6.raw.desc.json"
    };
//    run(raw_filenames, eb_filenames_256);
    run(raw_filenames, eb_filenames_512);
}