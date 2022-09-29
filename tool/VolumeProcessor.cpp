#include "VolumeProcessor.hpp"
#include <cmdline.hpp>

struct TaskNode{

    template<VolumeType type>
    struct Desc;

    template<>
    struct Desc<VolumeType::Grid_RAW>{

    };

    template<>
    struct Desc<VolumeType::Grid_SLICED>{

    };

    template<>
    struct Desc<VolumeType::Grid_BLOCKED_ENCODED>{

    };


    struct SrcNode{

    };

    struct DstNode{

        struct OpNode{

        };


    };


};

struct Process{

};




template<typename Voxel>
void Run(const std::vector<std::unique_ptr<Processor<Voxel>>>& processors){
    try{
        for(auto& processor : processors){
            processor->Convert();
        }
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
    }
}


template<typename Voxel>
void Parse(const Process& process){
    std::vector<std::unique_ptr<Processor<Voxel>>> processors;



    Run(processors);
}

void Parse(const Process& process){

    VoxelInfo voxel_info;

    if(voxel_info.type == vol::VoxelType::uint8 && voxel_info.format == VoxelFormat::R){
        Parse<VoxelRU8>(process);
    }
    else if(voxel_info.type == vol::VoxelType::uint16 && voxel_info.format == vol::VoxelFormat::R){
        Parse<VoxelRU16>(process);
    }

}

Process CreateFromFile(const std::string& filename, std::function<void(const char*)> errorCallback){


    return {};
}

void ParseFromFile(const std::string& filename){
    auto err_handle = [](const char* msg){
        std::cerr << msg << std::endl;
        exit(1);
    };
    auto process = CreateFromFile(filename, err_handle);
    Parse(process);
}

void PrintConfigFileFormat(){

}

int main(int argc, char** argv){
    try{
        cmdline::parser cmd;
        cmd.add<std::string>("config", 'c', "config json filename");
        cmd.add("print", 'p', "print config file format");

        cmd.parse_check(argc, argv);

        bool print = cmd.get<bool>("print");
        if (print) {
            PrintConfigFileFormat();
            return 0;
        }
        auto filename = cmd.get<std::string>("config");
        ParseFromFile(filename);
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
    }
    return 0;
}