
#include <VolumeUtils/Volume.hpp>

VOL_BEGIN


class RawGridVolumeFile {
    std::pair<RawGridVolumeDesc, bool> ExtractDescFromNameStr(const std::string &filename) {

    }

    std::pair<RawGridVolumeDesc, bool> ExtractDescFromFile(const std::string &filename) {
        // read from json file
    }

public:
    explicit RawGridVolumeFile(const std::string &filename) {
        auto [desc0, ok0] = ExtractDescFromNameStr(filename);
        if (ok0) {
            this->desc = desc0;
        } else {
            auto [desc1, ok1] = ExtractDescFromFile(filename);
            if (ok1) {
                this->desc = desc1;
            } else {
                throw VolumeFileOpenError("RawGridVolumeFile Open Error: " + filename);
            }
        }
    }

    [[nodiscard]]
    const RawGridVolumeDesc &GetVolumeDesc() const {
        return desc;
    }

    [[nodiscard]]
    const std::string &GetDataPath() const {
        return data_path;
    }

private:
    RawGridVolumeDesc desc;
    std::string data_path;
};

class RawGridVolumeReaderPrivate {
public:


};

class RawGridVolumeReaderPrivateIOStreamImpl : public RawGridVolumeReaderPrivate {
public:

};

//using mapping file will be better, but...
class RawGridVolumeReaderPrivateMappingFileImpl : public RawGridVolumeReaderPrivate {
public:

};


RawGridVolumeReader::RawGridVolumeReader(const std::string &filename) {

}

RawGridVolumeReader::~RawGridVolumeReader() {

}

size_t RawGridVolumeReader::ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, void *buf, size_t size) noexcept {

    return 0;
}

size_t RawGridVolumeReader::ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadFunc reader) noexcept {

    return 0;
}

const RawGridVolumeDesc &RawGridVolumeReader::GetVolumeDesc() const noexcept {

    return {};
}

class RawGridVolumeWriterPrivate{
public:

};

class RawGridVolumeWriterPrivateIOStreamImpl : public RawGridVolumeReaderPrivate {
public:

};

//using mapping file will be better, but...
class RawGridVolumeWriterPrivateMappingFileImpl : public RawGridVolumeReaderPrivate {
public:

};

RawGridVolumeWriter::RawGridVolumeWriter(const std::string &filename, const RawGridVolumeDesc& desc) {

}

RawGridVolumeWriter::~RawGridVolumeWriter() {

}

const RawGridVolumeDesc &RawGridVolumeWriter::GetVolumeDesc() const noexcept {
    return {};
}

void RawGridVolumeWriter::WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const void *buf, size_t size) noexcept {

}

void RawGridVolumeWriter::WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeWriteFunc writer) noexcept {

}




VOL_END

