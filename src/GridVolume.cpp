//
// Created by wyz on 2022/4/29.
//
#include <stdexcept>
#include <VolumeUtils/Volume.hpp>
#include <windows.h>
#include <iostream>
#include <locale.h>
#include <cassert>

VOL_BEGIN

class RawGridVolumePrivate{
public:
    RawGridVolumeDesc desc;
    HANDLE fileHandle = nullptr;
    HANDLE mapping = nullptr;
    void* ptr = nullptr;
    size_t fileSize = 0;
    bool mapped = false;
};
void PrintLastErrorMsg()
{
    setlocale(LC_ALL,"chs");
    DWORD dw = GetLastError();
    if(dw == ERROR_SUCCESS) return;
    char msg[ 512 ];
    //LPWSTR;
    FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            dw,
            MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
            msg,
            0, NULL );
    printf( "Win32 API Last Error Code: [%d] %s\n", dw, msg );
}
template<typename Voxel>
RawGridVolume<Voxel>::RawGridVolume(const RawGridVolumeDesc &desc, const std::string& filename) {
    _ = std::make_unique<RawGridVolumePrivate>();
    size_t fileSize = desc.extend.size() * sizeof(Voxel);
    try{
        if (filename.empty()) {
            //create in memory
            _->ptr = calloc(desc.extend.size(),sizeof(Voxel));
        }
        else {
            //create with mapping file
            bool newCreated = false;
            DWORD dwAttrib = GetFileAttributes(filename.c_str());
            if (!(dwAttrib != INVALID_FILE_ATTRIBUTES && 0 == (FILE_ATTRIBUTE_DIRECTORY & dwAttrib))) {
                newCreated = true;
            }

            HANDLE fileHandle = CreateFile(filename.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
                                           FILE_ATTRIBUTE_NORMAL, NULL);
            if (fileHandle == INVALID_HANDLE_VALUE || !fileHandle) {
                throw std::runtime_error("open file failed");
            }
            LARGE_INTEGER fs;
            fs.QuadPart = fileSize;

            if (newCreated) {
                SetFilePointer(fileHandle, fs.LowPart, &fs.HighPart, FILE_BEGIN);
                if (!SetEndOfFile(fileHandle)) {
                    throw std::runtime_error("set end of file failed");
                }
            }
            HANDLE mapping = CreateFileMapping(fileHandle, 0, PAGE_READWRITE, fs.HighPart, fs.LowPart, NULL);
            if (!mapping) {
                throw std::runtime_error("create file mapping failed");
            }
            LPVOID ptr = MapViewOfFile(mapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, fileSize);
            if (!ptr) {
                PrintLastErrorMsg();
                throw std::runtime_error("map file failed");
            }
            _->fileHandle = fileHandle;
            _->mapping = mapping;
            _->ptr = ptr;
            _->mapped = true;
        }
    }
    catch(const std::exception& err){
        std::cout<<err.what()<<std::endl;
    }
    _->fileSize = fileSize;
    _->desc = desc;
}

template<typename Voxel>
RawGridVolume<Voxel>::~RawGridVolume() {
    if(_->mapped){
        UnmapViewOfFile(_->ptr);
        CloseHandle(_->fileHandle);
        CloseHandle(_->mapping);
        PrintLastErrorMsg();
    }
    else{
        free(_->ptr);
    }
}

template<typename Voxel>
const RawGridVolumeDesc &RawGridVolume<Voxel>::GetGridVolumeDesc() const {
    return _->desc;
}

template<typename Voxel>
size_t RawGridVolume<Voxel>::ReadVoxels(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, Voxel *buf) {
    assert(buf);
    auto w = _->desc.extend.width;
    auto h = _->desc.extend.height;
    auto d = _->desc.extend.depth;
    int zz = dstZ - srcZ;
    int yy = dstY - srcY;
    int xx = dstX - srcX;
    size_t read_count = 0;
    for(int dz = 0; dz <= zz; dz++){
        int z = srcZ + dz;
        if(z < 0 || z >= d) continue;
        for(int dy = 0; dy <= yy; dy++){
            int y = srcY + dy;
            if(y < 0 || y >= h) continue;
            for(int dx = 0; dx <= xx; dx++){
                int x = srcX + dx;
                if(x < 0 || x >= w) continue;
                int offset = z * yy * xx + y * xx + x;
                buf[offset] = (*this)(x,y,z);
                read_count++;
            }
        }
    }
    return read_count;
}

/*
template<typename Voxel>
void GridVolume<Voxel>::UpdateVoxels(uint32_t srcX, uint32_t srcY, uint32_t srcZ, uint32_t dstX, uint32_t dstY, uint32_t dstZ, const Voxel *buf) {
    assert(buf);
    auto w = _->desc.extend.width;
    auto h = _->desc.extend.height;
    auto d = _->desc.extend.depth;
    if(srcX > dstX || dstX >= w || srcY > dstY || dstY >= h || srcZ > dstZ || dstZ >= d){
        throw std::invalid_argument("invalid range for update");
    }
    int zz = dstZ - srcZ;
    int yy = dstY - srcY;
    int xx = dstX - srcX;
    for(int dz = 0; dz <= zz; dz++){
        int z = srcZ + dz;
        if(z < 0 || z >= d) continue;
        for(int dy = 0; dy <= yy; dy++){
            int y = srcY + dy;
            if(y < 0 || y >= h) continue;
            for(int dx = 0; dx <= xx; dx++){
                int x = srcX + dx;
                if(x < 0 || x >= w) continue;
                size_t offset = (size_t)z * yy * xx + y * xx + x;
                (*this)(x,y,z) = buf[offset];
            }
        }
    }
}
*/

template<typename Voxel>
Voxel *RawGridVolume<Voxel>::GetData() {
    return static_cast<Voxel*>(_->ptr);
}

template<typename Voxel>
Voxel &RawGridVolume<Voxel>::operator()(int x, int y, int z) {
    auto w = _->desc.extend.width;
    auto h = _->desc.extend.height;
    auto d = _->desc.extend.depth;
    if(x < 0 || y < 0 || z < 0 || x >= w || y >= h || z >= d){
        throw std::out_of_range("out of volume");
    }
    size_t offset = (size_t)z*w*h + y * w + x;
    return *(static_cast<Voxel*>(_->ptr) + offset);
}


template class RawGridVolume<VoxelRU8>;
template class RawGridVolume<VoxelRU16>;


VOL_END