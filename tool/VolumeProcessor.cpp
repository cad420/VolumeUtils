#include "VolumeProcessor.hpp"
#include "Common.hpp"
#include <cmdline.hpp>
#include <json.hpp>
#include <fstream>
namespace{

    struct Process {

        static VolumeType StrToVolumeType(std::string_view str){
            if(str == "raw")
                return vol::VolumeType::Grid_RAW;
            else if(str == "sliced")
                return vol::VolumeType::Grid_SLICED;
            else if(str == "encoded_blocked")
                return vol::VolumeType::Grid_BLOCKED_ENCODED;
            else
                throw std::runtime_error("Invalid VolumeType Str");
        }

        struct TaskNode {

            struct SrcNode {
                VolumeRange range;
                VolumeType volume_type;
                VolumeDescT desc;
            };

            struct DstNode {

                struct OpNode {
                    bool down_sampling = false;
                    SamplingMethod down_sampling_method = SamplingMethod::MAX;
                    bool statistics = false;
                    std::string statistics_filename;
                    struct MappingNode{
                        MappingOps mapping_op;
                        double val;
                    };
                    std::vector<MappingNode> mapping_ops;
                    bool mapping = false;
                } op;

                VolumeType volume_type;
                VolumeDescT desc;
                std::string desc_filename;
                std::string vol_filename;
            };

            SrcNode src;
            std::vector<DstNode> dst;

        };

        template<VolumeType>
        struct Desc;

        template<>
        struct Desc<vol::VolumeType::Grid_RAW>{
            template<typename Json>
            static VolumeDescT Get(Json& j){
                VolumeDescT desc;
                desc.raw_desc.volume_name = "";


                return desc;
            }
        };

        template<>
        struct Desc<vol::VolumeType::Grid_SLICED>{
            template<typename Json>
            static VolumeDescT Get(Json& j){
                VolumeDescT desc;
                desc.sliced_desc.volume_name = "";


                return desc;
            }
        };

        template<>
        struct Desc<vol::VolumeType::Grid_BLOCKED_ENCODED>{
            template<typename Json>
            static VolumeDescT Get(Json& j){
                VolumeDescT desc;
                desc.blocked_desc.volume_name = "";


                return desc;
            }
        };

        template<typename Json>
        static VolumeDescT GetDesc(VolumeType type, Json& j){
            if(type == vol::VolumeType::Grid_RAW)
                return Desc<vol::VolumeType::Grid_RAW>::Get(j);
            else if(type == vol::VolumeType::Grid_SLICED)
                return Desc<vol::VolumeType::Grid_SLICED>::Get(j);
            else if(type == vol::VolumeType::Grid_BLOCKED_ENCODED)
                return Desc<vol::VolumeType::Grid_BLOCKED_ENCODED>::Get(j);
            else
                assert(false);
        }

        static SamplingMethod StrToSamplingMethod(std::string_view str){
            if(str == "max")
                return SamplingMethod::MAX;
            else if(str == "avg")
                return SamplingMethod::AVG;
            else
                throw std::runtime_error("Invalid SamplingMethod Str");
        }

        static MappingOps StrToMappingOp(std::string_view str){
            if(str == "add")
                return MappingOps::ADD;
            else if(str == "mul")
                return MappingOps::MUL;
            else if(str == "min")
                return MappingOps::MIN;
            else if(str == "max")
                return MappingOps::MAX;
            else
                throw std::runtime_error("Invalid MappingOps Str");
        }


        static std::unique_ptr<Process> CreateFromFile(const std::string& filename, std::function<void(const char*)> errorCallback){
            std::ifstream in(filename);
            if(!in.is_open()){
                errorCallback((std::string("Can't open file: ") + filename).c_str());
                return nullptr;
            }
            nlohmann::json j;
            in >> j;

            auto process = std::make_unique<Process>();
            auto& tasks = j.at("tasks");
            int task_count = j.at("task_count");

            process->memory_limit_gb = j.at("memory_limit_gb");

            for(int i = 0; i < task_count; i++){
                auto& task_node = process->tasks.emplace_back();

                auto& task = tasks.at("task" + std::to_string(i));
                auto& src = task.at("src");

                std::array<int, 6> range = src.at("range");
                task_node.src.range = {range[0], range[1], range[2], range[3], range[4], range[5]};
                task_node.src.volume_type = StrToVolumeType(src.at("volume_type"));
                task_node.src.desc = GetDesc(task_node.src.volume_type, src.at("desc"));


                int dst_count = task.at("dst_count");
                for(int k = 0; k < dst_count; k++){
                    auto& dst = task.at("dst" + std::to_string(k));

                    auto& dst_node = task_node.dst.emplace_back();
                    dst_node.desc_filename = dst.at("desc_filename");
                    dst_node.volume_type = StrToVolumeType(dst.at("volume_type"));
                    dst_node.desc = GetDesc(dst_node.volume_type, dst.at("desc"));

                    dst_node.vol_filename = dst.at("vol_filename");

                    auto& ops = dst.at("operations");
                    dst_node.op.down_sampling = ops.at("down_sampling") == "yes";
                    dst_node.op.down_sampling_method = StrToSamplingMethod(ops.at("down_sampling_method"));

                    dst_node.op.statistics = ops.at("statistics") == "yes";
                    dst_node.op.statistics_filename = ops.at("statistics_filename");

                    dst_node.op.mapping = ops.at("mapping") == "yes";
                    auto& mapping_ops = ops.at("mapping_ops");
                    for(auto it = mapping_ops.begin(); it != mapping_ops.end(); it++){
                        std::string key = it.key();
                        double value = it.value();
                        auto& mapping_op = dst_node.op.mapping_ops.emplace_back();
                        mapping_op.mapping_op = StrToMappingOp(key);
                        mapping_op.val = value;
                    }
                }
            }


            return process;
        }

        static std::unique_ptr<Process> CreateFromString(const std::string& context, std::function<void(const char*)> errorCallback){
            errorCallback("CreateFromString not imply now");
            return nullptr;
        }

        int memory_limit_gb = 0;
        std::vector<TaskNode> tasks;
    };
}




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


template<typename Voxel, VolumeType type>
void Parse(std::unique_ptr<Process> process){
    std::vector<std::unique_ptr<Processor<Voxel>>> processors;

    for(auto& task : process->tasks){
        auto& processor = processors.emplace_back(std::make_unique<VolumeProcessor<Voxel,type>>());
        // set source
        auto src_unit = typename Processor<Voxel>::Unit();
        src_unit.type = task.src.volume_type, src_unit.desc = task.src.desc;
        processor->SetSource(src_unit, task.src.range);
        // set target
        for(auto& dst : task.dst){
            auto dst_unit = typename Processor<Voxel>::Unit();
            dst_unit.desc_filename = dst.desc_filename;
            dst_unit.type = dst.volume_type;
            dst_unit.desc = dst.desc;
            if(dst.op.down_sampling){
                dst_unit.ops.AddDownSampling(DownSamplingOp<Voxel>(dst.op.down_sampling_method));
            }
            if(dst.op.statistics){
                dst_unit.ops.AddStatistics(StatisticsOp<Voxel>(dst.op.statistics_filename));
            }
            if(dst.op.mapping){
                MappingOp<Voxel> mapping_op;
                for(auto& mapping : dst.op.mapping_ops){
                    mapping_op.AddOp(mapping.mapping_op, mapping.val);
                }
                dst_unit.ops.AddMapping(std::move(mapping_op));
            }
        }
    }

    Run(processors);
}

void Parse(std::unique_ptr<Process> process){

    VoxelInfo voxel_info;
    VolumeType type;
    if(voxel_info.type == vol::VoxelType::uint8 && voxel_info.format == VoxelFormat::R){
        if(type == vol::VolumeType::Grid_RAW)
            Parse<VoxelRU8, vol::VolumeType::Grid_RAW>(std::move(process));
        else if(type == vol::VolumeType::Grid_SLICED)
            Parse<VoxelRU8, vol::VolumeType::Grid_SLICED>(std::move(process));
        else if(type == vol::VolumeType::Grid_BLOCKED_ENCODED)
            Parse<VoxelRU8, vol::VolumeType::Grid_BLOCKED_ENCODED>(std::move(process));
    }
    else if(voxel_info.type == vol::VoxelType::uint16 && voxel_info.format == vol::VoxelFormat::R){
        if(type == vol::VolumeType::Grid_RAW)
            Parse<VoxelRU16, vol::VolumeType::Grid_RAW>(std::move(process));
        else if(type == vol::VolumeType::Grid_SLICED)
            Parse<VoxelRU16, vol::VolumeType::Grid_SLICED>(std::move(process));
        else if(type == vol::VolumeType::Grid_BLOCKED_ENCODED)
            Parse<VoxelRU16, vol::VolumeType::Grid_BLOCKED_ENCODED>(std::move(process));
    }

}



void ParseFromFile(const std::string& filename){
    auto err_handle = [](const char* msg){
        std::cerr << msg << std::endl;
        exit(1);
    };
    auto process = Process::CreateFromFile(filename, err_handle);
    if(!process) return;
    Parse(std::move(process));
}

void PrintConfigFileFormat(){

}

int main(int argc, char** argv){
    try{
        cmdline::parser cmd;
        cmd.add("config", 'c', "config json filename");
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