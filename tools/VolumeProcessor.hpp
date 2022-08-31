#pragma once

#include <VolumeUtils/Volume.hpp>

using namespace vol;

template<VolumeType From,VolumeType To>
class VolumeProcessor;

template<>
class VolumeProcessor<VolumeType::Grid_SLICED,VolumeType::Grid_BLOCKED_ENCODED>{
public:

};


template<>
class VolumeProcessor<VolumeType::Grid_SLICED,VolumeType::Grid_RAW>{
public:

};

template<>
class VolumeProcessor<VolumeType::Grid_RAW,VolumeType::Grid_BLOCKED_ENCODED>{
public:

};

template<>
class VolumeProcessor<VolumeType::Grid_RAW,VolumeType::Grid_ENCODED>{
public:

};

template<>
class VolumeProcessor<VolumeType::Grid_BLOCKED_ENCODED,VolumeType::Grid_RAW>{
public:

};