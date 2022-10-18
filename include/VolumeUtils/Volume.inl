
enum class FileAccess{
    Read,
    Write,
    ReadWrite
};

enum class MapAccess{
    ReadOnly,
    ReadWrite
};

class IMappingFile{
public:

};

template<typename,bool>
class GridDataView;

template<typename, bool>
class SliceDataView;

template<typename T>
class SliceData{
public:
    SliceData(uint32_t sizeX, uint32_t sizeY, const T& init = T{})
            :sizeX(sizeX),sizeY(sizeY)
    {
        size_t count = (size_t)sizeX * sizeY;
        assert(count);
        data = static_cast<T*>(std::malloc(sizeof(T) * count));
        for(size_t i = 0; i < count; i++)
            data[i] = init;
    }

    SliceData(uint32_t sizeX, uint32_t sizeY, const T* srcP)
            :sizeX(sizeX), sizeY(sizeY)
    {
        size_t count = (size_t)sizeX * sizeY;
        assert(count);
        data = static_cast<T*>(std::malloc(sizeof(T) * count));
        std::memcpy(data, srcP, sizeof(T) * count);
    }

    ~SliceData(){
        if(data){
            free(data);
        }
    }

    const T& At(uint32_t x, uint32_t y) const{
        if(x >= sizeX || y >= sizeY){
            throw std::out_of_range("SliceData At out of range");
        }
        return this->operator()(x, y);
    }

    T& At(uint32_t x, uint32_t y){
        if(x >= sizeX || y >= sizeY){
            throw std::runtime_error("SliceData At out of range");
        }
        return this->operator()(x, y);
    }

    T& operator()(uint32_t x, uint32_t y) {
        return data[(size_t)y * sizeX + x];
    }

    const T& operator()(uint32_t x, uint32_t y) const {
        return data[(size_t)y * sizeX + x];
    }

    SliceDataView<T,false> GetSubView(uint32_t ox, uint32_t oy, uint32_t sx, uint32_t sy);

    SliceDataView<T,true> GetSubView(uint32_t ox, uint32_t oy, uint32_t sx, uint32_t sy) const;

    SliceDataView<T,true> GetConstSubView(uint32_t ox, uint32_t oy, uint32_t sx, uint32_t sy) const;

    auto Width() const{
        return sizeX;
    }

    auto Height() const{
        return sizeY;
    }

    size_t Size() const{
        return (size_t)sizeX * sizeY;
    }

    T* GetRawPtr(){
        return data;
    }

    const T* GetRawPtr() const {
        return data;
    }

private:
    T* data;
    uint32_t sizeX;
    uint32_t sizeY;
};

template<typename T, bool CONST = false>
class SliceDataView {

    using DataT = std::conditional_t<CONST, const SliceData<T>*, SliceData<T>*>;

public:
    SliceDataView(uint32_t oriX, uint32_t oriY, uint32_t sizeX, uint32_t sizeY, DataT srcP)
            :oriX(oriX), oriY(oriY), sizeX(sizeX), sizeY(sizeY), data(srcP)
    {

    }

    T& operator()(uint32_t x, uint32_t y){
        return data->operator()(oriX + x, oriY + y);
    }

    const T& operator()(uint32_t x, uint32_t y) const {
        return data->operator()(x, y);
    }

    const T& At(uint32_t x,uint32_t y) const{
        if(x >= sizeX || y >= sizeY){
            throw std::runtime_error("SliceDataView At out of range");
        }
        return this->operator()(x, y);
    }

    T& At(uint32_t x,uint32_t y){
        if(x >= sizeX || y >= sizeY){
            throw std::runtime_error("SliceDataView At out of range");
        }
        return this->operator()(x, y);
    }

    SliceDataView GetSubView(uint32_t ox, uint32_t oy, uint32_t sx, uint32_t sy){
        return SliceDataView(oriX + ox, oriY + oy, sx, sy, data);
    }
    SliceDataView<T,true> GetSubView(uint32_t ox, uint32_t oy, uint32_t sx, uint32_t sy) const{
        return SliceDataView<T,true>(oriX + ox, oriY + oy, sx, sy, data);
    }
    SliceDataView<T,true> GetConstSubView(uint32_t ox, uint32_t oy, uint32_t sx, uint32_t sy) const{
        return SliceDataView<T,true>(oriX + ox, oriY + oy, sx, sy, data);
    }

    bool IsValid() const{
        return data && sizeX && sizeY;
    }

    bool IsLinear() const{
        return oriX == 0 && oriY == 0;
    }

    auto Width() const{
        return sizeX;
    }

    auto Height() const{
        return sizeY;
    }
    T* GetRawPtr(){
        return data->GetRawPtr();
    }

    const T* GetRawPtr() const {
        return data->GetRawPtr();
    }
private:
    DataT data;
    uint32_t sizeX, sizeY;
    uint32_t oriX, oriY;
};

template<typename T>
SliceDataView<T, false> SliceData<T>::GetSubView(uint32_t ox, uint32_t oy, uint32_t sx, uint32_t sy) {
    return SliceDataView<T, false>(ox, oy, sx, sy, nullptr);
}
template<typename T>
SliceDataView<T,true> SliceData<T>::GetSubView(uint32_t ox, uint32_t oy, uint32_t sx, uint32_t sy) const{
    return SliceDataView<T, false>(ox, oy, sx, sy, nullptr);
}
template<typename T>
SliceDataView<T,true> SliceData<T>::GetConstSubView(uint32_t ox, uint32_t oy, uint32_t sx, uint32_t sy) const{
    return SliceDataView<T, false>(ox, oy, sx, sy, nullptr);
}

template<typename T>
class SliceView{
public:
    SliceView(uint32_t sizeX, uint32_t sizeY, const T* srcP)
            :sizeX(sizeX), sizeY(sizeY), data(srcP)
    {
        assert(sizeX && sizeY && srcP);
    }


    const T& operator()(uint32_t x, uint32_t y) const {
        return data[(size_t)y * sizeX + x];
    }


    const T& At(uint32_t x, uint32_t y) const {
        if(x >= sizeX || y >= sizeY){
            throw std::out_of_range("SliceView At out of range");
        }
        return this->operator()(x, y);
    }

    uint32_t sizeX, sizeY;
    const T* data;
};


template<typename T>
class GridData{
public:
    GridData(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, const T& init = T{})
            :sizeX(sizeX), sizeY(sizeY), sizeZ(sizeZ)
    {
        size_t count = (size_t)sizeX * sizeY * sizeZ;
        assert(count);
        data = static_cast<T*>(std::malloc(count * sizeof(T)));
        for(size_t i = 0; i < count; i++)
            data[i] = init;
    }

    GridData(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, const T* srcP)
            :sizeX(sizeX), sizeY(sizeY), sizeZ(sizeZ)
    {
        size_t count = (size_t)sizeX * sizeY * sizeZ;
        assert(count);
        data = static_cast<T*>(std::malloc(count * sizeof(T)));
        std::memcpy(data, srcP, count * sizeof(T));
    }

    SliceData<T> GetSliceDataX(uint32_t x) const{
        SliceData<T> slice(sizeY, sizeZ);
        for(uint32_t z = 0; z < sizeZ; z++){
            for(uint32_t y = 0; y < sizeY; y++){
                slice(y, z) = this->operator()(x, y, z);
            }
        }
        return slice;
    }

    SliceData<T> GetSliceDataY(uint32_t y) const{
        SliceData<T> slice(sizeZ, sizeX);
        for(uint32_t z = 0; z < sizeZ; z++){
            for(uint32_t x = 0; x < sizeX; x++){
                slice(z, x) = this->operator()(x, y, z);
            }
        }
        return slice;
    }

    SliceData<T> GetSliceDataZ(uint32_t z) const{
        SliceData<T> slice(sizeX, sizeY);
        for(uint32_t y = 0; y < sizeY; y++){
            for(uint32_t x = 0; x < sizeX; x++){
                slice(x, y) = this->operator()(x, y, z);
            }
        }
        return slice;
    }

    T& operator()(uint32_t x, uint32_t y, uint32_t z){
        return data[(size_t)z * sizeX * sizeY + (size_t)y * sizeX + x];
    }

    const T& operator()(uint32_t x, uint32_t y, uint32_t z) const {
        return data[(size_t)z * sizeX * sizeY + (size_t)y * sizeX + x];
    }

    T& At(uint32_t x, uint32_t y, uint32_t z){
        if(x >= sizeX || y >= sizeY || z >= sizeZ){
            throw std::out_of_range("GridData At out of range");
        }
        return this->operator()(x, y, z);
    }

    const T& At(uint32_t x, uint32_t y, uint32_t z) const {
        if(x >= sizeX || y >= sizeY || z >= sizeZ){
            throw std::out_of_range("GridData At out of range");
        }
        return this->operator()(x, y, z);
    }

    GridDataView<T,false> GetSubView(uint32_t ox, uint32_t oy, uint32_t oz, uint32_t sx, uint32_t sy, uint32_t sz);

    GridDataView<T,true> GetSubView(uint32_t ox, uint32_t oy, uint32_t oz, uint32_t sx, uint32_t sy, uint32_t sz) const;

    GridDataView<T,true> GetConstSubView(uint32_t ox, uint32_t oy, uint32_t oz, uint32_t sx, uint32_t sy, uint32_t sz) const;

    auto Width() const{
        return sizeX;
    }

    auto Height() const{
        return sizeY;
    }

    auto Depth() const{
        return sizeZ;
    }

    T* GetRawPtr(){
        return data;
    }

    const T* GetRawPtr() const {
        return data;
    }

    size_t Size() const{
        return (size_t)sizeX * sizeY * sizeZ;
    }

private:
    uint32_t sizeX;
    uint32_t sizeY;
    uint32_t sizeZ;
    T* data;
};

template<typename T, bool CONST = false>
class GridDataView {

    using DataT = std::conditional_t<CONST, const GridData<T>*, GridData<T>*>;

public:
    GridDataView(uint32_t oriX, uint32_t oriY, uint32_t oriZ, uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, DataT srcP)
            :oriX(oriX), oriY(oriY), oriZ(oriZ), sizeX(sizeX), sizeY(sizeY), sizeZ(sizeZ), data(srcP)
    {

    }
    T& operator()(uint32_t x, uint32_t y, uint32_t z){
        return data[size_t(z) * sizeX * sizeY + y * sizeX + x];
    }

    const T& operator()(uint32_t x, uint32_t y, uint32_t z) const {
        return data[size_t(z) * sizeX * sizeY + y * sizeX + x];
    }

    const T& At(uint32_t x, uint32_t y, uint32_t z) const{
        if(x < 0 || x >= sizeX || y < 0 || y >= sizeY || z < 0 || z >= sizeZ){
            throw std::runtime_error("GridDataView out of range");
        }
        return this->operator()(x, y, z);
    }

    T& At(uint32_t x, uint32_t y, uint32_t z){
        if(x < 0 || x >= sizeX || y < 0 || y >= sizeY || z < 0 || z >= sizeZ){
            throw std::runtime_error("GridDataView out of range");
        }
        size_t idx = size_t(z) * sizeX * sizeY + y * sizeX +x;
        return data[idx];
    }

    GridDataView<T,false> GetSubView(uint32_t ox, uint32_t oy, uint32_t oz, uint32_t sx, uint32_t sy, uint32_t sz){
        return GridDataView<T,false>(oriX + ox, oriY + oy, oriZ + oz, sx, sy, sz, data);
    }

    GridDataView<T,true> GetSubView(uint32_t ox, uint32_t oy, uint32_t oz, uint32_t sx, uint32_t sy, uint32_t sz) const {
        return GridDataView<T,true>(oriX + ox, oriY + oy, oriZ + oz, sx, sy, sz, data);
    }

    GridDataView<T,true> GetConstSubView(uint32_t ox, uint32_t oy, uint32_t oz, uint32_t sx, uint32_t sy, uint32_t sz) const {
        return GridDataView<T,true>(oriX + ox, oriY + oy, oriZ + oz, sx, sy, sz, data);
    }

    bool IsValid() const{
        return data && sizeX && sizeY && sizeZ;
    }

    bool IsLinear() const{
        return oriX == 0 && oriY == 0 && oriZ == 0;
    }

    auto Width() const{
        return sizeX;
    }

    auto Height() const{
        return sizeY;
    }

    auto Depth() const{
        return sizeZ;
    }

    T* GetRawPtr(){
        return data->GetRawPtr;
    }

    const T* GetRawPtr() const {
        return data->GetRawPtr;
    }
private:
    uint32_t oriX, oriY, oriZ;
    uint32_t sizeX, sizeY, sizeZ;
    T* data;
};

template<typename T>
GridDataView<T,false> GridData<T>::GetSubView(uint32_t ox, uint32_t oy, uint32_t oz, uint32_t sx, uint32_t sy, uint32_t sz) {
    return GridDataView<T,false>(ox, oy, oz, sx, sy, sz, data);
}

template<typename T>
GridDataView<T,true> GridData<T>::GetSubView(uint32_t ox, uint32_t oy, uint32_t oz, uint32_t sx, uint32_t sy, uint32_t sz) const {
    return GridDataView<T,true>(ox, oy, oz, sx, sy, sz, data);
}

template<typename T>
GridDataView<T,true> GridData<T>::GetConstSubView(uint32_t ox, uint32_t oy, uint32_t oz, uint32_t sx, uint32_t sy, uint32_t sz) const {
    return GridDataView<T,true>(ox, oy, oz, sx, sy, sz, data);
}

template<typename T>
class GridView{
public:
    GridView(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ,const T* srcP)
            :sizeX(sizeX),sizeY(sizeY),sizeZ(sizeZ),data(srcP)
    {}

    const T& operator()(uint32_t x, uint32_t y, uint32_t z) const{
        return data[(size_t)z * sizeX * sizeY + (size_t)y * sizeX + x];
    }


    const T& At(uint32_t x, uint32_t y, uint32_t z) const {
        if(x >= sizeX || y >= sizeY || z >= sizeZ){
            throw std::out_of_range("GridView At out of range");
        }
        return this->operator()(x, y, z);
    }

    const T* data;
    uint32_t sizeX;
    uint32_t sizeY;
    uint32_t sizeZ;
};
