#ifndef __rs_buffer_h__
#define __rs_buffer_h__

#include <vector>
#include <cstdint>

namespace rs_buffer
{
    class Buffer
    {
        // 缓冲区默认大小
        const int default_size = 1024;
    public:
        Buffer()
            : read_idx_(0), write_idx_(0), buffer_(1024)
        {
        }

        // 获取写位置
        void *getReadPos()
        {
        }

        // 获取读位置
        void *getWritePos()
        {
        }

        // 获取当前写位置之后可写空间
        uint64_t getBackWritableSize()
        {
        }

        // 获取当前读位置之前可写空间
        uint64_t getFrontWritableSize()
        {
        }

        // 获取可读数据大小
        uint64_t getReadableSize()
        {
        }

        // 写入数据
        // 将data数据写入指定长度到缓冲区中
        bool writeData(void *data, size_t len)
        {
        }

        // 读取数据
        // 从缓冲区读取指定长度数据到输出参数buf中
        bool readData(void *buf, size_t len)
        {
        }

        // 确定是否存在指定空间大小
        bool isEnough(size_t len)
        {
        }

        // 偏移读取指针
        void moveReadPtr(size_t len)
        {
        }

        // 偏移写入指针
        void moveWritePtr(size_t len)
        {
        }

        // 清理缓冲区
        void clear()
        {
        }

    private:
        std::vector<char> buffer_;
        uint64_t read_idx_;  // 读取起始位置（闭）
        uint64_t write_idx_; // 写入起始位置（闭）
    };
}

#endif