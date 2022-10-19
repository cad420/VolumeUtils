#include <VolumeUtils/Volume.hpp>
#include "../Common/LRU.hpp"
#include "../Common/Utils.hpp"
#include "../Common/Common.hpp"
#include <fstream>
#include <iostream>
#include <json.hpp>
#include <tiffio.h>

VOL_BEGIN

namespace{
    class SliceIOWrapper{
    public:
        struct SliceInfo{
            int width              = 0;
            int height             = 0;
            int samplers_per_pixel = 0;
            int bits_per_sampler   = 0;
            int rows_per_strip     = 1;
            bool compressed        = false;
        };
        virtual ~SliceIOWrapper() = default;
        virtual void Open(const std::string& filename, std::string_view mode)           = 0;
        virtual void Close()                                                            = 0;
        virtual const SliceInfo& GetSliceInfo() const noexcept                          = 0;
        virtual void SetSliceInfo(const SliceInfo&) noexcept                            = 0;
        /**
         * @brief read the whole slice data.
         */
        virtual size_t Read(void* buf, size_t size)                                             = 0;
        virtual size_t Read(int row, int nrows, void* buf, size_t size)                         = 0;
        virtual size_t Write(const void* buf, size_t size, bool overwrite)                      = 0;
        virtual size_t Write(int row, int nrows, const void* buf, size_t size, bool overwrite)  = 0;
    };

    class TIFIOWrapper : public SliceIOWrapper{
        std::string tif_filename;
        SliceInfo tif_slice_info;
        TIFF* tif = nullptr;
        std::vector<uint8_t> scanline_buffer;
    public:
        TIFIOWrapper() = default;

        ~TIFIOWrapper() override {
            Close();
        }

        void Open(const std::string& filename, std::string_view mode) override{
            if(tif) Close();
            tif = TIFFOpen(filename.c_str(), mode.data());
            if(!tif){
                throw VolumeFileOpenError("Open tif file failed : " + filename);
            }
            if(mode == "r") {
                TIFFGetField(tif, TIFFTAG_IMAGEWIDTH,      &tif_slice_info.width);
                TIFFGetField(tif, TIFFTAG_IMAGELENGTH,     &tif_slice_info.height);
                TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &tif_slice_info.samplers_per_pixel);
                TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE,   &tif_slice_info.bits_per_sampler);
                TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP,    &tif_slice_info.rows_per_strip);
                int cmp;
                TIFFGetField(tif, TIFFTAG_COMPRESSION, &cmp);
                tif_slice_info.compressed = cmp != COMPRESSION_NONE;
            }
            else if(mode == "w"){
                SetSliceInfo(tif_slice_info);
            }
            else{
                std::cerr << "Tif invalid open mode" << std::endl;
            }
        }

        void Close() noexcept override{
            if(!tif) return;
            TIFFClose(tif);
            tif = nullptr;
            tif_filename = "";
        }

        const SliceInfo& GetSliceInfo() const noexcept override{
            return tif_slice_info;
        }

        void SetSliceInfo(const SliceInfo& slice_info) noexcept override{
            tif_slice_info = slice_info;
            scanline_buffer.resize(slice_info.width * slice_info.bits_per_sampler * slice_info.samplers_per_pixel / 8);
            if(!tif) return;
            TIFFSetField(tif, TIFFTAG_ORIENTATION,  ORIENTATION_TOPLEFT);
            TIFFSetField(tif, TIFFTAG_PHOTOMETRIC,  PHOTOMETRIC_MINISBLACK);
            TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
            TIFFSetField(tif, TIFFTAG_IMAGEWIDTH,      tif_slice_info.width);
            TIFFSetField(tif, TIFFTAG_IMAGELENGTH,     tif_slice_info.height);
            TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, tif_slice_info.samplers_per_pixel);
            TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE,   tif_slice_info.bits_per_sampler);
            TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP,    tif_slice_info.rows_per_strip);
            TIFFSetField(tif, TIFFTAG_COMPRESSION,     tif_slice_info.compressed ? COMPRESSION_LZW : COMPRESSION_NONE);
        }

        size_t Read(void* buf, size_t size) override{
            assert(size >= tif_slice_info.width * tif_slice_info.height * tif_slice_info.samplers_per_pixel * tif_slice_info.bits_per_sampler / 8);
            return Read(0, tif_slice_info.height, buf, size);
        }

        size_t Read(int row, int nrows, void* buf, size_t size) override{
            if(!tif) return 0;
            auto dst_ptr = reinterpret_cast<uint8_t*>(buf);
            size_t read_size = 0;
            size_t pitch = tif_slice_info.width * tif_slice_info.samplers_per_pixel * tif_slice_info.bits_per_sampler / 8;
            for(int r = row; r < row + nrows; r++){
                size_t offset = (size_t)(r - row) * pitch;
                auto ret = TIFFReadScanline(tif, dst_ptr + offset, r, 0);
                if(ret == - 1){
                    std::cerr << "Tif read line error at row " << r << " in file " << tif_filename << std::endl;
                }
                read_size += pitch;
                assert(read_size <= size);
            }
            return read_size;
        }

        size_t Write(const void* buf, size_t size, bool overwrite) override{
            assert(size >= tif_slice_info.width * tif_slice_info.height * tif_slice_info.samplers_per_pixel * tif_slice_info.bits_per_sampler);
            return Write(0, tif_slice_info.height, buf, size, overwrite);
        }

        size_t Write(int row, int nrows, const void* buf, size_t size, bool overwrite) override{
            if(!buf || !size) return 0;
            auto src_ptr = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(buf));
            size_t write_size = 0;
            size_t pitch = tif_slice_info.width * tif_slice_info.samplers_per_pixel * tif_slice_info.bits_per_sampler / 8;
            for(int r = row; r < row + nrows; r++){
                size_t offset = (size_t)(r - row) * pitch;
                if(!overwrite){
                    throw std::runtime_error("Slice tif format not support overwrite/random write");
                }
                auto ret = TIFFWriteScanline(tif, src_ptr + offset, r, 0);
                if(ret == - 1){
                    std::cerr << "Tif write line error at row " << r << " in file " << tif_filename << std::endl;
                }
                write_size += pitch;
                assert(write_size <= size);
            }
            return write_size;
        }
    };

    std::unique_ptr<SliceIOWrapper> CreateSliceIOWrapperByExt(const std::string& ext){
        if(ext == ".tif" || ext == "tif"){
            return std::make_unique<TIFIOWrapper>();
        }
        return nullptr;
    }
}

class SlicedGridVolumeFile{
    const char* sliced       = "sliced";
    const char* slice_format = "slice_format";
    const char* volume_name  = "volume_name";
    const char* voxel_type   = "voxel_type";
    const char* voxel_format = "voxel_format";
    const char* extend       = "extend";
    const char* space        = "space";
    const char* axis         = "axis";
    const char* prefix       = "prefix";
    const char* postfix      = "postfix";
    const char* setw         = "setw";
public:
    bool Open(const std::string& filename){
        std::ifstream in(filename);
        if(!in.is_open()) return false;
        nlohmann::json j;
        in >> j;
        if(j.count("desc") == 0){
            std::cerr << "Json file error : not find desc" << std::endl;
            return false;
            //throw VolumeFileContextError(std::string("Json file not find item : ") + sliced);
        }
        auto& slice = j.at("desc");

        slice_data_format = slice.count(slice_format) == 0 ? ".none" : std::string(slice.at(slice_format));
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
        return true;
    }

    [[nodiscard]]
    const SlicedGridVolumeDesc& GetVolumeDesc() const{
        return desc;
    }

    [[nodiscard]]
    const std::string& GetSliceDataFormat() const{
        return slice_data_format;
    }

    bool Save(const std::string& filename, const SlicedGridVolumeDesc& volume_desc){
        // check desc when debug
        if(!CheckValidation(volume_desc)){
            std::cerr << "Invalid VolumeDesc" << std::endl;
            return false;
        }
        out.open(filename);
        if(!out.is_open()){
            std::cerr << "Open file failed : " << filename << std::endl;
            return false;
        }

        desc = volume_desc;

        nlohmann::json jj;

        auto& j = jj["desc"];

        j[slice_format] = slice_data_format;
        j[volume_name]  = desc.volume_name;
        j[voxel_type]   = VoxelTypeToStr(desc.voxel_info.type);
        j[voxel_format] = VoxelFormatToStr(desc.voxel_info.format);
        j[extend]       = {desc.extend.width, desc.extend.height, desc.extend.depth};
        j[space]        = {desc.space.x, desc.space.y, desc.space.z};
        j[axis]         = static_cast<int>(desc.axis);
        j[prefix]       = desc.prefix;
        j[postfix]      = desc.postfix;
        j[setw]         = desc.setw;

        assert(out.is_open());
        out << jj;
        out.flush();
        return true;
    }

    ~SlicedGridVolumeFile(){
        if(out.is_open()) out.close();
    }

    void SetSliceDataFormat(const std::string& fmt){
        slice_data_format = fmt;
    }

private:
    std::string slice_data_format;
    SlicedGridVolumeDesc desc;
    std::ofstream out;
};

class SlicedGridVolumeReaderPrivate{
public:
    SlicedGridVolumeDesc desc;
    std::unique_ptr<SliceIOWrapper> slice_io_wrapper;

    bool use_cache = false;
    int max_cached_slice_num = 0;
    std::vector<std::vector<uint8_t>> buffers;
    std::unique_ptr<lru_cache_t<int,uint8_t*>> slice_cache;

    size_t slice_bytes = 0;
    int slice_index = -1;
    std::vector<uint8_t> slice_data;

    SlicedGridVolumeFile file;
};


SlicedGridVolumeReader::SlicedGridVolumeReader(const std::string &filename) {
    _ = std::make_unique<SlicedGridVolumeReaderPrivate>();
//    SlicedGridVolumeFile file;
    if(!_->file.Open(filename)){
        throw VolumeFileOpenError("SlicedGridVolumeFile open failed : " + filename);
    }
    _->slice_io_wrapper = CreateSliceIOWrapperByExt(_->file.GetSliceDataFormat());
    if(_->slice_io_wrapper == nullptr){
        throw VolumeFileOpenError("SliceGridVolumeFile not supported slice data format : " + _->file.GetSliceDataFormat());
    }
    _->desc = _->file.GetVolumeDesc();
    _->desc.Generate();
    if(!CheckValidation(_->desc)){
        PrintVolumeDesc(_->desc);
        throw VolumeFileContextError("SliceGridVolumeFile context is not right : " + filename);
    }


    _->slice_bytes = (size_t)_->desc.extend.width * _->desc.extend.height * GetVoxelSize(_->desc.voxel_info);
    _->slice_data.resize(_->slice_bytes, 0);

    _->max_cached_slice_num = VolumeMemorySettings::MaxSlicedGridMemoryUsageBytes / _->slice_bytes;
}

SlicedGridVolumeReader::~SlicedGridVolumeReader() {
    // all is RAII and nothing need to do

}

size_t SlicedGridVolumeReader::ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, void *buf, size_t size) noexcept {
    size_t voxel_size = GetVoxelSize(_->desc.voxel_info);
    auto copy_func = GetCopyBitsFunc(voxel_size);
    return ReadVolumeData(srcX, srcY, srcZ, dstX, dstY, dstZ,
                          [dst_ptr = reinterpret_cast<uint8_t*>(buf), buf_size = size, width = dstX - srcX, height = dstY - srcY, &copy_func]
                          (int dx, int dy, int dz, const void* src, size_t ele_size)->size_t{
        size_t dst_offset = ((size_t)dz * width * height + (size_t)dy * width + dx) * ele_size;
        if(dst_offset >= buf_size) return 0;
        copy_func(reinterpret_cast<const uint8_t*>(src), dst_ptr + dst_offset);
        return ele_size;
    });
}

size_t SlicedGridVolumeReader::ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadFunc reader) noexcept {
    // invoke ReadSliceData
    if(srcX >= dstX || srcY >= dstY || srcZ >= dstZ) return 0;
    if(!reader) return 0;
    size_t read_size = 0;
    for(int slice_index = srcZ; slice_index < dstZ; slice_index++){
        auto ret = ReadSliceData(slice_index, srcX, srcY, dstX, dstY,
                                 [&reader, dz = slice_index - srcZ]
                                 (int dx,int dy,const void* src, size_t ele_size){
            return reader(dx, dy, dz, src, ele_size);
        });
        read_size += ret;
    }
    return read_size;
}

const SlicedGridVolumeDesc &SlicedGridVolumeReader::GetVolumeDesc() const noexcept {

    return _->desc;
}

size_t SlicedGridVolumeReader::ReadSliceData(int sliceIndex, void *buf, size_t size) noexcept {
    // maybe invoke next ReadSliceData to imply but would loss efficient
    if(sliceIndex < 0 || sliceIndex >= _->desc.extend.depth) return 0;
    if(!buf || !size) return 0;
    assert(_->desc.name_generator);
    assert(_->slice_bytes);
    size = std::min(size, _->slice_bytes);
    //maybe optimal...
    if(_->use_cache){
        assert(_->slice_cache);
        if(auto cached = _->slice_cache->get_value_optional(sliceIndex)){
            // copy from cache to buf
            std::copy(cached.value(), cached.value() + size, reinterpret_cast<uint8_t*>(buf));
            return size;
        }
        else{
            _->slice_io_wrapper->Open(_->desc.name_generator(sliceIndex) + _->file.GetSliceDataFormat(), "r");
            auto& updated = _->slice_cache->get_back();
            updated.first = sliceIndex;
            // check size
            auto ret = _->slice_io_wrapper->Read(updated.second,_->slice_bytes);
            assert(ret == _->slice_bytes);
            // todo log
            // copy to buf
            std::copy(updated.second, updated.second + size, reinterpret_cast<uint8_t*>(buf));
            _->slice_cache->get_value_ptr(sliceIndex);
            return size;
        }
    }
    else{
        if(_->slice_index == sliceIndex){
            std::memcpy(buf,_->slice_data.data(),size);
            return size;
        }
        else{
            _->slice_io_wrapper->Open(_->desc.name_generator(sliceIndex) + _->file.GetSliceDataFormat(), "r");
            auto ret =  _->slice_io_wrapper->Read(_->slice_data.data(),_->slice_bytes);
            assert(ret == _->slice_bytes);
            _->slice_index = sliceIndex;
            if(buf != _->slice_data.data())
                std::memcpy(buf,_->slice_data.data(),size);
            return size;
        }
    }
}

size_t SlicedGridVolumeReader::ReadSliceData(int sliceIndex, SliceReadFunc reader) noexcept {
    if(sliceIndex < 0 || sliceIndex >= _->desc.extend.depth) return 0;
    if(!reader) return 0;
    size_t read_size  = 0;
    size_t voxel_size = GetVoxelSize(_->desc.voxel_info);
    const uint8_t* src_ptr = nullptr;
    auto slice_w = _->desc.extend.width;
    auto slice_h = _->desc.extend.height;
    if(_->use_cache){
        assert(_->slice_cache);
        if(auto cached = _->slice_cache->get_value_optional(sliceIndex)){
            src_ptr = cached.value();
        }
        else{
            _->slice_io_wrapper->Open(_->desc.name_generator(sliceIndex) + _->file.GetSliceDataFormat(), "r");
            auto& updated = _->slice_cache->get_back();
            updated.first = sliceIndex;
            src_ptr = updated.second;
            auto ret = _->slice_io_wrapper->Read(updated.second, _->slice_bytes);
            assert(ret == _->slice_bytes);
            // change priority
            _->slice_cache->get_value_ptr(sliceIndex);
        }
    }
    else{
        if(_->slice_index == sliceIndex){
            src_ptr = _->slice_data.data();
        }
        else{
            _->slice_io_wrapper->Open(_->desc.name_generator(sliceIndex) + _->file.GetSliceDataFormat(), "r");
            auto ret =  _->slice_io_wrapper->Read(_->slice_data.data(),_->slice_bytes);
            assert(ret == _->slice_bytes);
            src_ptr = _->slice_data.data();
            _->slice_index = sliceIndex;
        }
    }
    // element range copy
    for(int row = 0; row < slice_h; row++){
        for(int col = 0; col < slice_w; col++){
            size_t src_offset = ((size_t)row * slice_w + col) * voxel_size;
            auto filled = reader(row, col, src_ptr + src_offset, voxel_size);
//            if(filled == 0) break;
            read_size += filled;
        }
    }
    return read_size;
}

size_t SlicedGridVolumeReader::ReadSliceData(int sliceIndex, int srcX, int srcY, int dstX, int dstY, void *buf, size_t size) noexcept {
    if(sliceIndex < 0 || sliceIndex >= _->desc.extend.depth) return 0;
    if(srcX >= dstX || srcY >= dstY) return 0;
    if(!buf || !size) return 0;

    size_t voxel_size = GetVoxelSize(_->desc.voxel_info);
    auto copy_func = GetCopyBitsFunc(voxel_size);

    //maybe invoke next ReadSliceData is more elegant...
    return ReadSliceData(sliceIndex, srcX, srcY, dstX, dstY,
                         [dst_ptr = reinterpret_cast<uint8_t*>(buf), buf_size = size, width = dstX - srcX, &copy_func]
                         (int dx, int dy, const void* src, size_t ele_size)->size_t{
        size_t dst_offset = ((size_t)dy * width + dx) * ele_size;
        if(dst_offset >= buf_size) return 0;
        copy_func(reinterpret_cast<const uint8_t*>(src),dst_ptr + dst_offset);
        return ele_size;
    });

}

size_t SlicedGridVolumeReader::ReadSliceData(int sliceIndex, int srcX, int srcY, int dstX, int dstY, SliceReadFunc reader) noexcept {
    if(sliceIndex < 0 || sliceIndex >= _->desc.extend.depth) return 0;
    if(srcX >= dstX || srcY >= dstY) return 0;
    if(!reader) return 0;

    size_t read_size  = 0;
    size_t voxel_size = GetVoxelSize(_->desc.voxel_info);
    auto slice_w = _->desc.extend.width;
    auto slice_h = _->desc.extend.height;
    auto valid_pos = [width = slice_w, height = slice_h](int row, int col){
        return row >= 0 && col >= 0 && row < height && col < width;
    };

    auto ret = ReadSliceData(sliceIndex,_->slice_data.data(),_->slice_data.size());
    // must equal or throw an exception
    assert(ret == _->slice_data.size());
    auto src_ptr = reinterpret_cast<const uint8_t*>(_->slice_data.data());

    // element range copy, maybe slow but can support more features
    for(int row = srcY; row < dstY; row++){
        for(int col = srcX; col < dstX; col++){
            if(!valid_pos(row, col)) continue;
            size_t src_offset = ((size_t)row * slice_w + col) * voxel_size;
            auto filled = reader(col - srcX, row - srcY, src_ptr + src_offset, voxel_size);
//            if(filled == 0) break;
            read_size += filled;
        }
    }
    return read_size;
}

void SlicedGridVolumeReader::SetUseCached(bool useCached) {
    if(useCached && _->buffers.empty()){
        //init cache buffers
        _->buffers.resize(_->max_cached_slice_num);
        for(auto& b : _->buffers){
            b.resize(_->slice_bytes, 0);
        }
        assert(_->slice_cache == nullptr);
        _->slice_cache = std::make_unique<lru_cache_t<int,uint8_t*>>(_->max_cached_slice_num);
        // just add buffers with invalid slice index
        for(int i = 0; i < _->max_cached_slice_num; i++){
            _->slice_cache->emplace_back(-i, _->buffers[i].data());
        }
    }

    _->use_cache = useCached;
}

bool SlicedGridVolumeReader::GetIfUseCached() const {
    return _->use_cache;
}

class SlicedGridVolumeWriterPrivate{
public:
    SlicedGridVolumeDesc desc;
    SliceIOWrapper::SliceInfo slice_info;
    std::unique_ptr<SliceIOWrapper> slice_io_wrapper;


    size_t slice_bytes = 0;
    int slice_index = -1;
    std::vector<uint8_t> slice_data;
    std::vector<bool> dirty;

    SlicedGridVolumeFile file;
};

constexpr const char* DefaultSliceDataFormat = ".tif";

SlicedGridVolumeWriter::SlicedGridVolumeWriter(const std::string &filename, const SlicedGridVolumeDesc& desc) {
    if(!CheckValidation(desc)){
        throw VolumeFileContextError("Invalid SlicedGridVolumeDesc");
    }
    _ = std::make_unique<SlicedGridVolumeWriterPrivate>();
    _->desc = desc;
    _->desc.Generate();
    _->file.SetSliceDataFormat(DefaultSliceDataFormat);
    _->file.Save(filename, desc);

    _->slice_info = {.width = (int)desc.extend.width, .height = (int)desc.extend.height,
                     .samplers_per_pixel = GetVoxelSampleCount(desc.voxel_info.format),
                     .bits_per_sampler = GetVoxelBits(desc.voxel_info.type),
                     .rows_per_strip = 6,
                     .compressed = true};
    _->slice_io_wrapper = CreateSliceIOWrapperByExt(DefaultSliceDataFormat);

    _->slice_bytes = GetVoxelSize(_->desc.voxel_info) * _->desc.extend.width * _->desc.extend.height;
    _->slice_data.resize(_->slice_bytes, 0);
    _->dirty.resize(_->desc.extend.height, false);
}

SlicedGridVolumeWriter::~SlicedGridVolumeWriter() {
    // flush dirty data
    Flush();
}


const SlicedGridVolumeDesc &SlicedGridVolumeWriter::GetVolumeDesc() const noexcept {
    return _->desc;
}

void SlicedGridVolumeWriter::WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeWriteFunc writer) noexcept {
    if(srcX >= dstX || srcY >= dstY || srcZ >= dstZ) return;
    if(!writer) return;
    for(int slice_index = srcZ; slice_index < dstZ; slice_index++){
        WriteSliceData(slice_index, srcX, srcY, dstX, dstY,
                        [&writer, dz = slice_index - srcZ]
                             (int dx,int dy,void* dst, size_t ele_size){
                         return writer(dx, dy, dz, dst, ele_size);
                        });
    }
}

void SlicedGridVolumeWriter::WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const void *buf, size_t size) noexcept {
    size_t voxel_size = GetVoxelSize(_->desc.voxel_info);
    auto copy_func = GetCopyBitsFunc(voxel_size);
    return WriteVolumeData(srcX, srcY, srcZ, dstX, dstY, dstZ,
                           [src_ptr = reinterpret_cast<const uint8_t*>(buf), buf_size = size,
                            width = dstX - srcX, height = dstY - srcY, &copy_func]
                           (int dx, int dy, int dz, void* dst, size_t ele_size){
        size_t src_offset = ((size_t)dz * width * height + (size_t)dy * width + dx) * ele_size;
        copy_func(src_ptr + src_offset, reinterpret_cast<uint8_t*>(dst));
    });
}

void SlicedGridVolumeWriter::WriteSliceData(int sliceIndex, SliceWriteFunc writer) noexcept {
    if(sliceIndex < 0 || sliceIndex >= _->desc.extend.depth) return;
    if(!writer) return;
    if(sliceIndex != _->slice_index){
        // flush old slice data
        Flush();
        // open new slice
        _->slice_io_wrapper->SetSliceInfo(_->slice_info);
        _->slice_io_wrapper->Open(_->desc.name_generator(sliceIndex) + _->file.GetSliceDataFormat(), "w");
    }
    size_t voxel_size = GetVoxelSize(_->desc.voxel_info);
    auto slice_w = _->desc.extend.width;
    auto slice_h = _->desc.extend.height;
    auto dst_ptr = _->slice_data.data();
    for(int row = 0; row < slice_h; row++){
        for(int col = 0; col < slice_w; col++){
            size_t dst_offset = ((size_t)row * slice_w + col) * voxel_size;
            writer(row, col, dst_ptr + dst_offset, voxel_size);
        }
    }

    auto ret = _->slice_io_wrapper->Write(_->slice_data.data(),_->slice_bytes, true);
    assert(ret == _->slice_bytes);

    _->dirty.assign(_->dirty.size(), false);
    _->slice_index = sliceIndex;
}

void SlicedGridVolumeWriter::WriteSliceData(int sliceIndex, const void *buf, size_t size) noexcept {
    if(sliceIndex < 0 || sliceIndex >= _->desc.extend.depth) return;
    if(!buf || !size) return;

    // not invoke WriteSliceData above and use below code is more efficient
    if(sliceIndex != _->slice_index){
        // flush old slice data
        Flush();
        // open new slice
        _->slice_io_wrapper->SetSliceInfo(_->slice_info);
        _->slice_io_wrapper->Open(_->desc.name_generator(sliceIndex) + _->file.GetSliceDataFormat(), "w");
    }

    size_t write_size = std::min(size, _->slice_bytes);

    std::memcpy(_->slice_data.data(), buf, write_size);

    auto ret = _->slice_io_wrapper->Write(_->slice_data.data(),_->slice_bytes, true);
    assert(ret == _->slice_bytes);

    _->dirty.assign(_->dirty.size(), false);
    _->slice_index = sliceIndex;
}

void SlicedGridVolumeWriter::WriteSliceData(int sliceIndex, int srcX, int srcY, int dstX, int dstY, SliceWriteFunc writer) noexcept {
    if(sliceIndex < 0 || sliceIndex >= _->desc.extend.depth) return;
    if(srcX >= dstX || srcY >= dstY) return;
    if(!writer) return;

    if(sliceIndex != _->slice_index){
        // flush old slice data
        Flush();
        // open new slice
        _->slice_io_wrapper->Close();
        _->slice_io_wrapper->SetSliceInfo(_->slice_info);
        _->slice_io_wrapper->Open(_->desc.name_generator(sliceIndex) + _->file.GetSliceDataFormat(), "w");
    }
    size_t voxel_size = GetVoxelSize(_->desc.voxel_info);
    auto slice_w = _->desc.extend.width;
    auto slice_h = _->desc.extend.height;
    auto valid_pos = [width = slice_w, height = slice_h](int row, int col){
        return row >= 0 && col >= 0 && row < height && col < width;
    };
    auto dst_ptr = _->slice_data.data();
    for(int row = srcY; row < dstY; row++){
        for(int col = srcX; col < dstX; col++){
            if(!valid_pos(row, col)) continue;
            size_t dst_offset = ((size_t)row * slice_w + col) * voxel_size;
            writer(col - srcX, row - srcY, dst_ptr + dst_offset, voxel_size);
            _->dirty[row] = true;
        }
    }
    _->slice_index = sliceIndex;
}

void SlicedGridVolumeWriter::WriteSliceData(int sliceIndex, int srcX, int srcY, int dstX, int dstY, const void *buf, size_t size) noexcept {
    size_t voxel_size = GetVoxelSize(_->desc.voxel_info);
    auto copy_func = GetCopyBitsFunc(voxel_size);
    return WriteSliceData(sliceIndex, srcX, srcY, dstX, dstY,
                          [src_ptr = reinterpret_cast<const uint8_t*>(buf), width = dstX - srcX, buf_size = size, &copy_func]
                          (int dx, int dy, void* dst, size_t ele_size){
        size_t src_offset = ((size_t)dy * width + dx) * ele_size;
        if(src_offset >= buf_size) return;
        copy_func(src_ptr + src_offset, reinterpret_cast<uint8_t*>(dst));
    });
}

void SlicedGridVolumeWriter::Flush() noexcept {
    if(_->slice_index == -1) return;
    assert(_->slice_index >= 0 && _->slice_index < _->desc.extend.depth);
    size_t pitch = _->desc.extend.width * GetVoxelSize(_->desc.voxel_info);
    for(int row = 0; row < _->desc.extend.height; row++){
        if(!_->dirty[row]) continue;
        size_t offset = row * pitch;
        _->slice_io_wrapper->Write(row, 1, _->slice_data.data() + offset, pitch, false | _->desc.overwrite);
        _->dirty[row] = false;
    }
    std::memset(_->slice_data.data(), 0, _->slice_data.size());
}




VOL_END