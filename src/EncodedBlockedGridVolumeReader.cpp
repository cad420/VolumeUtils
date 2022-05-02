//
// Created by wyz on 2022/5/2.
//
#include <VolumeUtils/Volume.hpp>
#include "EncodedBlockedGridVolumeFile.hpp"
class EncodedBlockedGridVolumeReaderPrivate{
public:
    EncodedBlockedGridVolumeFile file;
};

EncodedBlockedGridVolumeReader::EncodedBlockedGridVolumeReader(const std::string &filename) {

}

EncodedBlockedGridVolumeReader::~EncodedBlockedGridVolumeReader() {

}

void EncodedBlockedGridVolumeReader::Close() {

}

const EncodedBlockedGridVolumeDesc &EncodedBlockedGridVolumeReader::GetEncodedBlockedGridVolumeDesc() const {
    return _->file.header.desc;
}

void EncodedBlockedGridVolumeReader::ReadBlockData(const BlockIndex &, void *buf) {

}

void EncodedBlockedGridVolumeReader::ReadRawBlockData(const BlockIndex &, Packet &) {

}

void EncodedBlockedGridVolumeReader::ReadRawBlockData(const BlockIndex &, Packets &) {

}
