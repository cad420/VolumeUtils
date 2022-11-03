#include "VolumeProcessor.hpp"
#include "Common.hpp"
#include <cmdline.hpp>
#include <json.hpp>
#include <fstream>

extern size_t GetAvailMemSize();

//todo logger to file

namespace{

    struct Process {
        inline static const char* sliced           = "sliced";
        inline static const char* slice_format     = "slice_format";
        inline static const char* volume_name      = "volume_name";
        inline static const char* voxel_type       = "voxel_type";
        inline static const char* voxel_format     = "voxel_format";
        inline static const char* extend           = "extend";
        inline static const char* space            = "space";
        inline static const char* axis             = "axis";
        inline static const char* prefix           = "prefix";
        inline static const char* postfix          = "postfix";
        inline static const char* setw             = "setw";
        inline static const char* block_length     = "block_length";
        inline static const char* padding          = "padding";
        inline static const char* volume_codec     = "volume_codec";
        inline static const char* data_path        = "data_path";
        inline static const char* volume_desc_file = "volume_desc_file";

        static VolumeType StrToVolumeType(std::string_view str){
            // todo change to const var
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
                std::string volume_desc_file;
                VolumeDescT desc;
                VoxelInfo voxel_info;
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
            static void Get(Json& raw, RawGridVolumeDesc& desc){
                std::string name = raw.at(volume_name);
                desc.volume_name = raw.count(volume_name) == 0 ? "none" : std::string(raw.at(volume_name));
                desc.voxel_info.type = StrToVoxelType(raw.count(voxel_type) == 0 ? "unknown" : raw.at(voxel_type));
                desc.voxel_info.format = StrToVoxelFormat(raw.count(voxel_format) == 0 ? "none" : raw.at(voxel_format));
                if(raw.count(extend) != 0){
                    std::array<int, 3> shape = raw.at(extend);
                    desc.extend = {(uint32_t)shape[0], (uint32_t)shape[1], (uint32_t)shape[2]};
                }
                if(raw.count(space) != 0){
                    std::array<float, 3> sp = raw.at(space);
                    desc.space = {sp[0], sp[1], sp[2]};
                }
                desc.data_path = raw.at(data_path);
            }
        };

        template<>
        struct Desc<vol::VolumeType::Grid_SLICED>{
            template<typename Json>
            static void Get(Json& slice, SlicedGridVolumeDesc& desc){
                desc.volume_name = slice.count(volume_name) == 0 ? "none" : std::string(slice.at(volume_name));
                desc.voxel_info.type = StrToVoxelType(slice.count(voxel_type) == 0 ? "unknown" : slice.at(voxel_type));
                desc.voxel_info.format = StrToVoxelFormat(slice.count(voxel_format) == 0 ? "none" : slice.at(voxel_format));
                if(slice.count(extend) != 0){
                    std::array<int, 3> shape = slice.at(extend);
                    desc.extend = {(uint32_t)shape[0], (uint32_t)shape[1], (uint32_t)shape[2]};
                }
                if(slice.count(space) != 0){
                    std::array<float, 3> sp = slice.at(space);
                    desc.space = {sp[0], sp[1], sp[2]};
                }
                desc.axis = slice.count(axis) == 0 ? SliceAxis::AXIS_Z : static_cast<SliceAxis>(slice.at(axis));
                desc.prefix = slice.count(prefix) == 0 ? "" : slice.at(prefix);
                desc.postfix = slice.count(postfix) == 0 ? "" : slice.at(postfix);
                desc.setw = slice.count(setw) == 0 ? 0 : (int)slice.at(setw);
            }
        };

        template<>
        struct Desc<vol::VolumeType::Grid_BLOCKED_ENCODED>{
            template<typename Json>
            static void Get(Json& eb, EncodedBlockedGridVolumeDesc& desc){
                desc.volume_name = eb.count(volume_name) == 0 ? "none" : std::string(eb.at(volume_name));
                desc.voxel_info.type = StrToVoxelType(eb.count(voxel_type) == 0 ? "unknown" : eb.at(voxel_type));
                desc.voxel_info.format = StrToVoxelFormat(eb.count(voxel_format) == 0 ? "none" : eb.at(voxel_format));
                if(eb.count(extend) != 0){
                    std::array<int, 3> shape = eb.at(extend);
                    desc.extend = {(uint32_t)shape[0], (uint32_t)shape[1], (uint32_t)shape[2]};
                }
                if(eb.count(space) != 0){
                    std::array<float, 3> sp = eb.at(space);
                    desc.space = {sp[0], sp[1], sp[2]};
                }
                desc.block_length = eb.at(block_length);
                desc.padding = eb.at(padding);
                desc.codec = StrToGridVolumeDataCodec(eb.at(volume_codec));
                desc.data_path = eb.at(data_path);
            }
        };

        template<typename Json>
        static void GetDesc(VolumeType type, Json& j, VolumeDescT& desc){
            if(type == vol::VolumeType::Grid_RAW)
                Desc<vol::VolumeType::Grid_RAW>::Get(j, desc.raw_desc);
            else if(type == vol::VolumeType::Grid_SLICED)
                Desc<vol::VolumeType::Grid_SLICED>::Get(j, desc.sliced_desc);
            else if(type == vol::VolumeType::Grid_BLOCKED_ENCODED)
                Desc<vol::VolumeType::Grid_BLOCKED_ENCODED>::Get(j, desc.encoded_blocked_desc);
            else
                assert(false);
        }


        static void GetDescFromFile(VolumeType type, const std::string& filename, VolumeDescT& desc){
            std::ifstream in(filename);
            if(!in.is_open()){
                throw std::runtime_error("Failed to open desc json file : " + filename);
            }
            nlohmann::json j;
            in >> j;

            GetDesc(type, j.at("desc"), desc);
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
            // warning
            if(filename.substr(filename.length() - 13) != ".process.json"){
                std::cerr << "Warning: filename not end with .process.json, this may not be config file." << std::endl;
            }

            std::stringstream ss;
            ss << in.rdbuf();

            return CreateFromString(ss.str(), std::move(errorCallback));
        }

        static std::unique_ptr<Process> CreateFromString(const std::string& context, std::function<void(const char*)> errorCallback){
            nlohmann::json j = nlohmann::json::parse(context);

            auto process = std::make_unique<Process>();
            auto& tasks = j.at("tasks");
            int task_count = j.at("task_count");

            process->memory_limit_gb = std::min<int>(GetAvailMemSize() >> 30, j.at("memory_limit_gb"));

            for(int i = 0; i < task_count; i++){
                auto& task_node = process->tasks.emplace_back();

                auto& task = tasks.at("task" + std::to_string(i));
                auto& src = task.at("src");

                std::array<int, 6> range = src.at("range");
                task_node.src.range = {range[0], range[1], range[2], range[3], range[4], range[5]};
                task_node.src.volume_type = StrToVolumeType(src.at("volume_type"));
                task_node.src.volume_desc_file = src.at(volume_desc_file);
                task_node.src.voxel_info.type = StrToVoxelType(src.at(voxel_type));
                task_node.src.voxel_info.format = StrToVoxelFormat(src.at(voxel_format));

                int dst_count = task.at("dst_count");
                for(int k = 0; k < dst_count; k++){
                    auto& dst = task.at("dst" + std::to_string(k));

                    auto& dst_node = task_node.dst.emplace_back();
                    dst_node.desc_filename = dst.at("desc_filename");
                    dst_node.volume_type = StrToVolumeType(dst.at("volume_type"));
                    dst_node.desc.Set(dst_node.volume_type);
                    GetDesc(dst_node.volume_type, dst.at("desc"),  dst_node.desc);

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


            return std::move(process);
        }

        int memory_limit_gb = 0;
        std::vector<TaskNode> tasks;
    };
}




template<typename Voxel>
void Run(std::unique_ptr<Processor<Voxel>> processor){
    try{
        processor->Convert();
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
    }
}

template<typename Voxel, VolumeType type>
void Parse(const Process::TaskNode& task){
    std::unique_ptr<Processor<Voxel>> processor = std::make_unique<VolumeProcessor<Voxel, type>>();
    // set source
    auto src_unit = typename Processor<Voxel>::Unit();
    src_unit.type = task.src.volume_type;
    src_unit.desc_filename = task.src.volume_desc_file;
    processor->SetSource(src_unit, task.src.range);
    // set target
    for(auto& dst : task.dst){
        auto dst_unit = typename Processor<Voxel>::Unit();
        dst_unit.desc_filename = dst.desc_filename;
        dst_unit.type = dst.volume_type;
        dst_unit.desc.Set(dst.volume_type, dst.desc);
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
        processor->AddTarget(dst_unit);
    }

    Run(std::move(processor));
}


void Parse(std::unique_ptr<Process> process){
    if(process->tasks.empty()) return;
    for(auto& task : process->tasks){
        VoxelInfo voxel_info = task.src.voxel_info;
        VolumeType type = task.src.volume_type;
        if (voxel_info.type == vol::VoxelType::uint8 && voxel_info.format == VoxelFormat::R) {
            if (type == vol::VolumeType::Grid_RAW)
                Parse<VoxelRU8, vol::VolumeType::Grid_RAW>(task);
            else if (type == vol::VolumeType::Grid_SLICED)
                Parse<VoxelRU8, vol::VolumeType::Grid_SLICED>(task);
            else if (type == vol::VolumeType::Grid_BLOCKED_ENCODED)
                Parse<VoxelRU8, vol::VolumeType::Grid_BLOCKED_ENCODED>(task);
        } else if (voxel_info.type == vol::VoxelType::uint16 && voxel_info.format == vol::VoxelFormat::R) {
            if (type == vol::VolumeType::Grid_RAW)
                Parse<VoxelRU16, vol::VolumeType::Grid_RAW>(task);
            else if (type == vol::VolumeType::Grid_SLICED)
                Parse<VoxelRU16, vol::VolumeType::Grid_SLICED>(task);
            else if (type == vol::VolumeType::Grid_BLOCKED_ENCODED)
                Parse<VoxelRU16, vol::VolumeType::Grid_BLOCKED_ENCODED>(task);
        }
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
    std::cerr << "TODO..." << std::endl;
}

int main(int argc, char** argv){
    try{
        cmdline::parser cmd;
        cmd.add<std::string>("config", 'c', "config json filename");
        cmd.add("print", 'p', "print config file format");

        cmd.parse_check(argc, argv);

        bool print = cmd.exist("print");
        if (print) {
            PrintConfigFileFormat();
            return 0;
        }

        if(!cmd.exist("config")){
            throw std::runtime_error(cmd.usage().c_str());
        }
        auto filename = cmd.get<std::string>("config");
        ParseFromFile(filename);
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
    }
    return 0;
}