#pragma once

#include <VolumeUtils/Volume.hpp>
#include <optional>
#include <variant>
#include <queue>
#include <unordered_map>
using namespace vol;

enum VolumeTypeBits : int{
    Unknown = 0,
    Grid_RAW_BIT = 0b1,
    Grid_SLICED_BIT = 0b10,
    Grid_BLOCKED_ENCODED_BIT = 0b100,
    Grid_ENCODED_BIT = 0b1000,
    Grid_BLOCKED_BIT = 0b10000
};

inline constexpr VolumeTypeBits GetVolumeTypeBits(VolumeType type){
    switch (type) {
        case VolumeType::Grid_RAW : return VolumeTypeBits::Grid_RAW_BIT;
        case VolumeType::Grid_SLICED : return VolumeTypeBits::Grid_SLICED_BIT;
        case VolumeType::Grid_BLOCKED_ENCODED : return VolumeTypeBits::Grid_BLOCKED_ENCODED_BIT;
        case VolumeType::Grid_ENCODED : return VolumeTypeBits::Grid_ENCODED_BIT;
        case VolumeType::Grid_BLOCKED : return VolumeTypeBits::Grid_BLOCKED_BIT;
        default: return VolumeTypeBits::Unknown;
    }
}
enum class Operation : int{
    None = 0,
    DownSampling = 1,
    Statistics = 2,
    Mapping = 4
};

template<typename Voxel>
class DownSamplingOp{
public:
    enum SamplingMethod{
        AVG,
        MAX
    };
    using ArgT = std::conditional_t<sizeof(Voxel) >= 9, const Voxel&, Voxel>;
    using Func = std::function<Voxel(ArgT, ArgT)>;

    DownSamplingOp( SamplingMethod method){
        if(method == AVG){
            func = [](ArgT a, ArgT b)->Voxel{
                if constexpr(IsVoxelTypeInteger(Voxel::type)){
                    // unsigned int
                    return {(a.r > b.r) ? b.r + ((a.r - b.r) >> 1) : a.r + ((b.r - a.r) >> 1)};
                }
                else{
                    // float
                    return {a.r + (b.r - a.r) * 0.5};
                }
            };
        }
        else if(method == MAX){
            func = [](ArgT a, ArgT b)->Voxel{
                return {a.r > b.r ? a.r : b.r};
            };
        }
    }
    DownSamplingOp(Func f)
    :func(std::move(f))
    {
    }

    Func GetOp() const{
        return func;
    }

private:
    Func func;

};
template<typename Voxel>
class StatisticsOp{
public:
    using ArgT = std::conditional_t<sizeof(Voxel) >= 9, const Voxel&, Voxel>;
    void AddVoxel(ArgT voxel){
        his[voxel.r] += 1;
    }
private:
    std::unordered_map<typename Voxel::VoxelDataType,size_t> his;
};

template<typename Voxel>
class MappingOp{
public:
    enum Ops{
        ADD,
        MUL,
        MIN,
        MAX,
        MAP
    };
    using ArgT = std::conditional_t<sizeof(Voxel) >= 9, const Voxel&, Voxel>;
    using Func = std::function<Voxel(ArgT)>;

    MappingOp()
    {
        func = [](ArgT a)->Voxel{
            return a;
        };
    }


    void AddOp(Func fun){
        Func f = [func = std::move(this->func), fn = std::move(fun)](ArgT a)->Voxel{
            return fn(func(a));
        };
        this->func = std::move(f);
    }

    void AddOp(Ops op, ArgT rhs){
        if(op == ADD)
            AddOp_ADD(rhs);
        else if(op == MUL)
            AddOp_MUL(rhs);
        else if(op == MAX)
            AddOp_MAX(rhs);
        else if(op == MIN)
            AddOp_MIN(rhs);
    }
private:
    void AddOp_ADD(ArgT rhs){
        Func f = [func = std::move(this->func), rhs](ArgT a)->Voxel{
            return {func(a).r + rhs.r};
        };
        this->func = std::move(f);
    }

    void AddOp_MUL(ArgT rhs){
        Func f = [func = std::move(this->func), rhs](ArgT a)->Voxel{
            return {func(a).r * rhs.r};
        };
        this->func = std::move(f);
    }

    void AddOp_MIN(ArgT rhs){
        Func f = [func = std::move(this->func), rhs](ArgT a)->Voxel{
            return {std::min(func(a).r, rhs.r)};
        };
        this->func = std::move(f);
    }

    void AddOp_MAX(ArgT rhs){
        Func f = [func = std::move(this->func), rhs](ArgT a)->Voxel{
            return {std::max(func(a).r, rhs.r)};
        };
        this->func = std::move(f);
    }

private:
    Func func;
};

template<typename Voxel>
class Operations{
public:
    std::variant<DownSamplingOp<Voxel>,StatisticsOp<Voxel>,MappingOp<Voxel>> op_func;
    Operation op;
    Operations* next = nullptr;
};


using VolumeDescT = VolumeFileDesc;

template<typename Voxel, VolumeType SrcT>
class VolumeProcessor;

struct VolumeRange{
    int src_x, src_y, src_z;
    int dst_x, dst_y, dst_z;
};

template<typename Voxel>
class Processor{
public:
    static_assert(Voxel::format == VoxelFormat::R, "current only support single R channel volume data");
    virtual ~Processor() = default;

    struct Unit{
        Unit(){

        }
        ~Unit(){

        }
        Unit(const Unit&){

        }
        Unit& operator=(const Unit& ){

            return *this;
        }
        VolumeType type;
        VolumeDescT desc;
        std::vector<Operations<Voxel>> ops;
    };

    virtual Processor& SetSource(const Unit& u, const VolumeRange& range) = 0;

    virtual Processor& AddTarget(const Unit& u) = 0;

    virtual void Convert() = 0;
};

template<typename Voxel>
class RawVolumeProcessorPrivate;
template<typename Voxel>
class VolumeProcessor<Voxel, VolumeType::Grid_RAW> : public Processor<Voxel>{
public:
    using Self = VolumeProcessor<Voxel, VolumeType::Grid_RAW>;
    using Unit = typename Processor<Voxel>::Unit;

    VolumeProcessor();

    ~VolumeProcessor() override;

    Self& SetSource(const Unit& u, const VolumeRange& range) override;

    Self& AddTarget(const Unit& u) override;

    void Convert() override;

private:
    std::unique_ptr<RawVolumeProcessorPrivate<Voxel>> _;
};
template<typename Voxel>
using RawVolumeProcessor = VolumeProcessor<Voxel, VolumeType::Grid_RAW>;

template<typename Voxel>
class SlicedVolumeProcessorPrivate;
template<typename Voxel>
class VolumeProcessor<Voxel, VolumeType::Grid_SLICED> : public Processor<Voxel>{
public:
    using Self = VolumeProcessor<Voxel, VolumeType::Grid_SLICED>;
    using Unit = typename Processor<Voxel>::Unit;

    VolumeProcessor();

    ~VolumeProcessor();

    Self& SetSource(const Unit& u, const VolumeRange& range) override;

    Self& AddTarget(const Unit& u) override;

    void Convert() override;
private:
    std::unique_ptr<SlicedVolumeProcessorPrivate<Voxel>> _;
};
template<typename Voxel>
using SlicedVolumeProcesspr = VolumeProcessor<Voxel, VolumeType::Grid_SLICED>;

template<typename Voxel>
class BlockedEncodedVolumeProcessorPrivate;
template<typename Voxel>
class VolumeProcessor<Voxel, VolumeType::Grid_BLOCKED_ENCODED> : public Processor<Voxel>{
public:
    using Self = VolumeProcessor<Voxel, VolumeType::Grid_BLOCKED_ENCODED>;
    using Unit = typename Processor<Voxel>::Unit;

    VolumeProcessor();

    ~VolumeProcessor();

    Self& SetSource(const Unit& u, const VolumeRange& range) override;

    Self& AddTarget(const Unit& u) override;

    void Convert() override;
private:
    std::unique_ptr<BlockedEncodedVolumeProcessorPrivate<Voxel>> _;
};
template<typename Voxel>
using BlockedEncodedVolumeProcessor = VolumeProcessor<Voxel, VolumeType::Grid_BLOCKED_ENCODED>;