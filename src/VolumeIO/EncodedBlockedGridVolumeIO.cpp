#include <VolumeUtils/Volume.hpp>

VOL_BEGIN

class EncodedBlockedGridVolumeReaderPrivate{
public:

};

EncodedBlockedGridVolumeReader::EncodedBlockedGridVolumeReader(const std::string &filename) {

}

EncodedBlockedGridVolumeReader::~EncodedBlockedGridVolumeReader() {

}

const EncodedBlockedGridVolumeDesc &EncodedBlockedGridVolumeReader::GetVolumeDesc() const noexcept {

}

size_t EncodedBlockedGridVolumeReader::ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, void *buf, size_t size) noexcept {

}

size_t EncodedBlockedGridVolumeReader::ReadVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeReadFunc reader) noexcept {

}

size_t EncodedBlockedGridVolumeReader::ReadBlockData(const BlockIndex &blockIndex, void *buf, size_t size) noexcept {

}

size_t EncodedBlockedGridVolumeReader::ReadBlockData(const BlockIndex &blockIndex, VolumeReadFunc reader) noexcept {

}

size_t EncodedBlockedGridVolumeReader::ReadEncodedBlockData(const BlockIndex &blockIndex, Packets &packets) noexcept {

}

size_t EncodedBlockedGridVolumeReader::ReadEncodedBlockData(const BlockIndex &blockIndex, void *buf, size_t size) noexcept {

}

class EncodedBlockedGridVolumeWriterPrivate{
public:

};

EncodedBlockedGridVolumeWriter::EncodedBlockedGridVolumeWriter(const std::string &filename) {

}

EncodedBlockedGridVolumeWriter::~EncodedBlockedGridVolumeWriter() {

}

void EncodedBlockedGridVolumeWriter::SetVolumeDesc(const EncodedBlockedGridVolumeDesc &) noexcept {

}

const EncodedBlockedGridVolumeDesc &EncodedBlockedGridVolumeWriter::GetVolumeDesc() const noexcept {
    return {};
}

void EncodedBlockedGridVolumeWriter::WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, VolumeWriteFunc writer) noexcept {

}

void EncodedBlockedGridVolumeWriter::WriteVolumeData(int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, const void *buf, size_t size) noexcept {

}

void EncodedBlockedGridVolumeWriter::WriteBlockData(const BlockIndex &blockIndex, VolumeWriteFunc writer) noexcept {

}

void EncodedBlockedGridVolumeWriter::WriteBlockData(const BlockIndex &blockIndex, const void *buf, size_t size) noexcept {

}

void EncodedBlockedGridVolumeWriter::WriteEncodedBlockData(const BlockIndex &blockIndex, const Packets &packets) noexcept {

}

void EncodedBlockedGridVolumeWriter::WriteEncodedBlockData(const BlockIndex &blockIndex, const void *buf, size_t size) noexcept {

}



VOL_END