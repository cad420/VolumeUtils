
#include <VolumeUtils/Volume.hpp>

VOL_BEGIN

class RawGridVolumeFile{
    std::pair<RawGridVolumeDesc,bool> ExtractDescFromNameStr(const std::string& filename){

    }
    std::pair<RawGridVolumeDesc,bool> ExtractDescFromFile(const std::string& filename){

    }
public:
    RawGridVolumeFile(const std::string& filename){
        auto [desc0, ok0] = ExtractDescFromNameStr(filename);
        if(ok0){
            this->desc = desc0;
        }
        else{
            auto [desc1, ok1] = ExtractDescFromFile(filename);
            if(ok1){
                this->desc = desc1;
            }
            else{
                throw VolumeFileOpenError("RawGridVolumeFile Open Error: " + filename);
            }
        }
    }
    const RawGridVolumeDesc& GetVolumeDesc() const{
        return desc;
    }
    const std::string& GetDataPath() const{
        return data_path;
    }
private:
    RawGridVolumeDesc desc;
    std::string data_path;
};

struct RawGridVolumeReaderPrivate{
    RawGridVolumeDesc desc;
    //using mapping file will be better, but...

};

RawGridVolumeReader::RawGridVolumeReader(const std::string &filename) {

}

RawGridVolumeReader::~RawGridVolumeReader() {

}

size_t RawGridVolumeReader::ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, void *buf, size_t size) noexcept {

    return 0;
}

size_t RawGridVolumeReader::ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadWriteFunc reader) noexcept {

    return 0;
}

const RawGridVolumeDesc &RawGridVolumeReader::GetVolumeDesc() const noexcept {

    return {};
}

struct RawGridVolumeWriterPrivate{

};

RawGridVolumeWriter::RawGridVolumeWriter(const std::string &filename) {

}

RawGridVolumeWriter::~RawGridVolumeWriter() {

}

void RawGridVolumeWriter::SetVolumeDesc(const RawGridVolumeDesc &) noexcept {

}

void RawGridVolumeWriter::WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const void *buf, size_t size) noexcept {

}

void RawGridVolumeWriter::WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadWriteFunc writer) noexcept {

}


VOL_END

