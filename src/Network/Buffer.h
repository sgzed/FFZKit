#ifndef FFZKIT_BUFFER_H
#define FFZKIT_BUFFER_H

#include <string>
#include <memory>
#include "Util/util.h"
#include "Util/ResourcePool.h"

namespace FFZKit {

//缓存基类
class Buffer : public noncopyable {
public:
    using Ptr = std::shared_ptr<Buffer>;

    Buffer() = default;
    virtual ~Buffer() = default;

    virtual char* data() const = 0;
    virtual size_t size() const = 0;

    virtual std::string toString() const {
        return std::string(data(), size());
    }

    virtual size_t getCapacity() const {
        return size();
    }

private:
    //对象个数统计
    ObjectStatistic<Buffer> statistic_;
};

//指针式缓存对象
class BufferRaw : public Buffer {
public:
    using Ptr = std::shared_ptr<BufferRaw>;

    static Ptr create(size_t size = 0);

    ~BufferRaw() override {
        if(data_)
            delete[] data_;
    }

    char* data() const override {
        return data_;
    }

    size_t size() const override {
        return size_;
    }

    size_t getCapacity() const override {
        return capacity_;
    }

    void setCapacity(size_t capacity) {
        if (data_) {
            do {
                //realloc
                if(capacity > capacity_) {
                    break;
                }

                if(2 * 1024 > capacity_) {
                    //小于2K，不重复开辟内存，直接复用
                    return;
                }

                if(2 * capacity > capacity_) {
                    //请求的内存大于当前内存的一半，那么也复用  
                    return;
                }

            } while (false);

            delete[] data_;
        }

        data_ = new char[capacity];
        capacity_ = capacity;
    }

    //设置有效数据大小
    void setSize(size_t size) {
        if (size > capacity_) {
            throw std::invalid_argument("Buffer::setSize out of range");
        }
        size_ = size;
    }

    void assign(const char* data, size_t size) {
        if(size <= 0) {
            size = strlen(data);
        }
        setCapacity(size + 1);
        memcpy(data_, data, size);
        data_[size] = '\0';
        setSize(size);
    }

protected:
    friend class ResourcePool_I<BufferRaw>;

    BufferRaw(size_t capacity = 0) {
        if (capacity) {
            setCapacity(capacity);
        }
    }

    BufferRaw(const char *data, size_t size = 0) {
        assign(data, size);
    }

private:
    size_t size_ = 0; 
    size_t capacity_ = 0;
    char* data_  = nullptr;
    ObjectStatistic<BufferRaw> statistic_;
};

} // namespace FFZKit


#endif //FFZKIT_BUFFER_H