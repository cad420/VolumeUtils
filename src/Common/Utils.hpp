#pragma once

#include <functional>
#include <chrono>
#include <thread>
#include <mutex>
#define START_TIMER auto __start = std::chrono::steady_clock::now();

#define STOP_TIMER(msg) \
auto __end = std::chrono::steady_clock::now();\
auto __t = std::chrono::duration_cast<std::chrono::milliseconds>(__end-__start);\
std::cout<<msg<<" cost time "<<__t.count()<<" ms"<<std::endl;

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


class progress_bar_t{
public:
    explicit progress_bar_t(int width,char comp = '#',char incomp = ' ')
            :percent(0.0),display_width(width),complete(comp),incomplete(incomp),
             start_time(std::chrono::steady_clock::now())
    {}

    void set_percent(double per){
        percent = per;
    }
    void reset_time(){
        start_time = std::chrono::steady_clock::now();
    }
    auto get_time_ms() const noexcept{
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
    }
    void display() const{
        auto p = static_cast<int>(display_width * percent * 0.01);
        auto cost_t = get_time_ms();
        std::cout<<"[";
        for(int i = 0; i < display_width; i++){
            if(i < p)
                std::cout<<complete;
            else if(i == p)
                std::cout<<">";
            else
                std::cout<<incomplete;
        }
        std::cout<<"] "<<static_cast<int>(percent)<<"%, "<<cost_t<<" ms.   \r";
        std::cout.flush();
    }

    void done(){
        set_percent(100.0);
        display();
        std::cout<<std::endl;
    }
private:
    double percent;
    int display_width;
    char complete;
    char incomplete;
    std::chrono::steady_clock::time_point start_time;
};

inline int actual_worker_count(int worker_count) noexcept
{
    if(worker_count <= 0)
        worker_count += static_cast<int>(std::thread::hardware_concurrency());
    return (std::max)(1, worker_count);
}

template<typename Iterable, typename Func>
void parallel_foreach(
        Iterable &&iterable, const Func &func, int worker_count = 0)
{
    std::mutex it_mutex;
    auto it = iterable.begin();
    auto end = iterable.end();
    auto next_item = [&]() -> decltype(std::make_optional(*it))
    {
        std::lock_guard lk(it_mutex);
        if(it == end)
            return std::nullopt;
        return std::make_optional(*it++);
    };

    std::mutex except_mutex;
    std::exception_ptr except_ptr = nullptr;

    worker_count = actual_worker_count(worker_count);

    auto worker_func = [&](int thread_index)
    {
        for(;;)
        {
            auto item = next_item();
            if(!item)
                break;

            try
            {
                func(thread_index, *item);
            }
            catch(...)
            {
                std::lock_guard lk(except_mutex);
                if(!except_ptr)
                    except_ptr = std::current_exception();
            }

            std::lock_guard lk(except_mutex);
            if(except_ptr)
                break;
        }
    };

    std::vector<std::thread> workers;
    for(int i = 0; i < worker_count; ++i)
        workers.emplace_back(worker_func, i);

    for(auto &w : workers)
        w.join();

    if(except_ptr)
        std::rethrow_exception(except_ptr);
}

template<typename T, typename Func>
void parallel_forrange(T beg, T end, Func &&func, int worker_count = 0)
{
    std::mutex it_mutex;
    T it = beg;
    auto next_item = [&]() -> std::optional<T>
    {
        std::lock_guard lk(it_mutex);
        if(it == end)
            return std::nullopt;
        return std::make_optional(it++);
    };

    std::mutex except_mutex;
    std::exception_ptr except_ptr = nullptr;

    worker_count = actual_worker_count(worker_count);

    auto worker_func = [&](int thread_index)
    {
        for(;;)
        {
            auto item = next_item();
            if(!item)
                break;

            try
            {
                func(thread_index, *item);
            }
            catch(...)
            {
                std::lock_guard lk(except_mutex);
                if(!except_ptr)
                    except_ptr = std::current_exception();
            }

            std::lock_guard lk(except_mutex);
            if(except_ptr)
                break;
        }
    };

    std::vector<std::thread> workers;
    for(int i = 0; i < worker_count; ++i)
        workers.emplace_back(worker_func, i);

    for(auto &w : workers)
        w.join();

    if(except_ptr)
        std::rethrow_exception(except_ptr);
}