//
// Created by wyz on 2022/4/29.
//
#include <VolumeUtils/Volume.hpp>
#include <iostream>
#include <fstream>

using namespace vol;
int main(){

    using VoxelT = Voxel<VoxelType::uint8,VoxelFormat::R>;
    using VolumeT = GridVolume<VoxelT>;
    std::string filename = "E:/Volume/foot_256_256_256_uint8.raw";
    int table[256]={0};
    int count = 0;
    {
        VolumeT volume({{256,   256,   256},
                        {0.01f, 0.01f, 0.01f}}, filename);
        count = volume.GetGridVolumeDesc().extend.size();
        std::vector<uint8_t> buffer(count,0);
        volume.ReadVoxels(0,0,0,255,255,255,reinterpret_cast<VoxelT*>(buffer.data()));
        for (int i = 0; i < count; i++) {
            table[buffer[i]]++;
        }
        for (int i = 0; i < 256; i++) {
            std::cout << i << " " << table[i] << std::endl;
        }
    }

    std::vector<uint8_t> data(count);
    std::ifstream in(filename,std::ios::binary);
    if(!in.is_open()){
        std::cout<<"can't open file"<<std::endl;
        exit(1);
    }
    in.read(reinterpret_cast<char*>(data.data()),count);
    in.close();
    for(int i = 0;i< count;i++){
        table[data[i]]--;
    }
    bool ok = true;
    for(int i = 0;i<256;i++){
        if(table[i]){
            std::cerr<<"not equal "<<i<<" delta "<<table[i]<<std::endl;
            ok = false;
        }
    }
    if(ok){
        std::cout<<"all the same"<<std::endl;
    }
}