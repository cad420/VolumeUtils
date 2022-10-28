#pragma once

#include <functional>

class ScopeGuard{
public:
    ScopeGuard(std::function<void()> f)
    :f(std::move(f))
    {}

    ~ScopeGuard(){
        if(f)
            f();
    }

    void Dismiss(){
        f = nullptr;
    }
private:
    std::function<void()> f;
};

template<int Bits>
inline void CopyBits(const uint8_t* src, uint8_t* dst){
    for(int i = 0; i < Bits; i++){
        *(dst++) = *(src++);
    }
}

inline void CopyNBits(const uint8_t* src, uint8_t* dst, size_t bits){
    for(int i = 0; i < bits; i++){
        *(dst++) = *(src++);
    }
}

template<>
inline void CopyBits<1>(const uint8_t* src, uint8_t* dst){
    *dst = *src;
}


template<>
inline void CopyBits<2>(const uint8_t* src, uint8_t* dst){
    auto psrc = reinterpret_cast<const uint16_t*>(src);
    auto pdst = reinterpret_cast<uint16_t*>(dst);
    *dst = *src;
}

template<>
inline void CopyBits<4>(const uint8_t* src, uint8_t* dst){
    auto psrc = reinterpret_cast<const uint32_t*>(src);
    auto pdst = reinterpret_cast<uint32_t*>(dst);
    *dst = *src;
}

template<>
inline void CopyBits<8>(const uint8_t* src, uint8_t* dst){
    auto psrc = reinterpret_cast<const uint64_t*>(src);
    auto pdst = reinterpret_cast<uint64_t*>(dst);
    *dst = *src;
}

inline auto GetCopyBitsFunc(size_t voxel_size){
    std::function<void(const uint8_t*,uint8_t*)> f;
    switch (voxel_size) {
        case 1 : f = CopyBits<1>;
            break;
        case 2 : f = CopyBits<2>;
            break;
        case 4 : f = CopyBits<4>;
            break;
        case 8 : f = CopyBits<8>;
            break;
        default: f = [voxel_size](auto && PH1, auto && PH2) { return CopyNBits(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2), voxel_size); };
    }
    return f;
}

inline std::string ConvertStrToLower(std::string_view str){
    std::string ret;
    ret.reserve(str.length());
    for(auto ch : str){
        if(std::isupper(ch))
            ret.push_back(static_cast<char>(std::tolower(ch)));
        else
            ret.push_back(ch);
    }
    return ret;
}