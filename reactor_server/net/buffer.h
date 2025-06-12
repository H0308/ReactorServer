#ifndef __rs_buffer_h__
#define __rs_buffer_h__

#include <vector>
#include <cstdint>
#include <cassert>
#include <string>

namespace rs_buffer
{
    class Buffer
    {
        // 缓冲区默认大小
        const int default_size = 1024;

    public:
        Buffer()
            : read_idx_(0), write_idx_(0), buffer_(default_size)
        {
        }

        // 获取第一个元素所在的位置
        char *getStartPos()
        {
            return &(*buffer_.begin());
        }

        // 获取写位置
        char *getReadPos()
        {
            char *start = getStartPos();
            return start + read_idx_;
        }

        // 获取读位置
        char *getWritePos()
        {
            char *start = getStartPos();
            return start + write_idx_;
        }

        // 获取第一个元素所在的位置
        const char *getStartPos() const
        {
            return &(*buffer_.begin());
        }

        // 获取写位置
        const char *getReadPos() const
        {
            const char *start = getStartPos();
            return start + read_idx_;
        }

        // 获取读位置
        const char *getWritePos() const
        {
            const char *start = getStartPos();
            return start + write_idx_;
        }

        // 获取当前写位置之后可写空间
        uint64_t getBackWritableSize()
        {
            // 左闭右开
            return buffer_.size() - write_idx_;
        }

        // 获取当前读位置之前可写空间
        uint64_t getFrontWritableSize()
        {
            return read_idx_;
        }

        // 获取可读数据大小
        uint64_t getReadableSize() const
        {
            return write_idx_ - read_idx_;
        }

        // 写任意数据——不移动写入指针
        // 将data数据写入指定长度到缓冲区中
        void write_noMove(void *data, size_t len)
        {
            // 1. 确保空间足够
            setEnoughSpace(len);
            const char *data1 = static_cast<const char *>(data);
            // 2. 写入数据
            std::copy(data1, data1 + len, getWritePos());
        }

        // 写任意数据——移动写入指针
        void write_move(void *data, size_t len)
        {
            write_noMove(data, len);
            moveWritePtr(len);
        }

        // 写入字符串数据——不移动写入指针
        void write_noMove(const std::string &data, size_t len)
        {
            char *data1 = const_cast<char *>(data.c_str());
            write_noMove(data1, len);
        }

        // 写入字符串数据——移动写入指针
        void write_move(const std::string &data, size_t len)
        {
            char *data1 = const_cast<char *>(data.c_str());
            write_move(data1, len);
        }

        // 写入其他缓冲区的数据——不移动写入指针
        void write_noMove(const Buffer &data, size_t len)
        {
            write_noMove(data.getReadPos(), data.getReadableSize());
        }

        // 写入其他缓冲区的数据——移动写入指针
        void write_move(const Buffer &data, size_t len)
        {
            write_move(data.getReadPos(), data.getReadableSize());
        }

        // 读取任意数据——不移动指针
        // 从缓冲区读取指定长度数据到输出参数buf中
        void read_noMove(void *buf, size_t len)
        {
            // 处理空指针
            if(!buf)
                return;
            // 1. 确保要求的长度小于可读空间大小
            uint64_t readable_size = getReadableSize();
            assert(len <= readable_size);

            // 2. 将指定长度数据拷贝到输出参数中
            char *buf1 = static_cast<char *>(buf);
            char *start = getReadPos();
            std::copy(start, start + len, buf1);
        }

        // 读取任意数据——移动指针
        void read_move(void *buf, size_t len)
        {
            read_noMove(buf, len);
            moveReadPtr(len);
        }

        // 读取数据存入字符串——不移动指针
        void read_noMove(std::string &buf, size_t len)
        {
            read_noMove(&(buf[0]), len);
        }

        // 读取数据存入字符串——移动指针
        void read_move(std::string &buf, size_t len)
        {
            read_move(&(buf[0]), len);
        }

        // 查找\r\n
        uint64_t getCRLF_1(std::string &buf)
        {
            return buf.find("\r\n");
        }

        // 查找\n
        uint64_t getCRLF_2(std::string &buf)
        {
            return buf.find("\n");
        }

        // 读取一行数据（存入字符串，会保存换行符）——不移动指针
        void readLine_noMove(std::string &buf)
        {
            // 基于可读部分创建临时字符串
            std::string readable_data(getReadPos(), getReadPos() + getReadableSize());
            // 先获取\r\n，找不到再获取\n
            uint64_t pos = getCRLF_1(readable_data);

            if (pos != std::string::npos)
            {
                // 确保不会越界
                if (pos + 2 <= getReadableSize())
                {
                    std::string str(getReadPos(), getReadPos() + pos + 2);
                    buf = str;
                }
            }
            else
            {
                pos = getCRLF_2(readable_data);
                if (pos != std::string::npos && pos + 1 <= getReadableSize())
                {
                    std::string str(getReadPos(), getReadPos() + pos + 1);
                    buf = str;
                }
            }
        }

        // 读取一行数据（存入字符串，会保存换行符）——移动指针
        void readLine_move(std::string &buf)
        {
            readLine_noMove(buf);
            moveReadPtr(buf.size());
        }

        // 确定是否存在指定空间大小
        void setEnoughSpace(size_t len)
        {
            // 三种情况：
            // 1. 需要的空间大小小于写位置之后的空间大小，说明空间足够，函数不进行任何行为
            // 2. 否则，需要的空间大小小于写位置之后的空间大小与读位置之前的空间大小，说明需要挪动已有的数据到空间起始位置
            // 3. 否则，需要进行扩容，但不进行数据挪动
            uint64_t back_size = getBackWritableSize();
            uint64_t front_size = getFrontWritableSize();

            if (len <= back_size)
                return;
            else if (len <= back_size + front_size)
            {
                char *start_pos = getReadPos();
                // 挪动数据
                uint64_t readable_size = getReadableSize();
                std::copy(start_pos, start_pos + readable_size, buffer_.begin());
            }
            else
            {
                // 从写位置开始计算，此时写位置就代表当前已经占用的空间大小
                buffer_.resize(write_idx_ + len);
            }
        }

        // 偏移读取指针
        void moveReadPtr(size_t len)
        {
            if(len == 0)
                return ;
            assert(len <= getReadableSize());
            read_idx_ += len;
        }

        // 偏移写入指针
        void moveWritePtr(size_t len)
        {
            // 只判断后方是否有足够空间
            assert(len <= getBackWritableSize());
            write_idx_ += len;
        }

        // 清理缓冲区
        // 更新位置标记为0即可
        void clear()
        {
            read_idx_ = 0;
            write_idx_ = 0;
        }

    private:
        std::vector<char> buffer_;
        uint64_t read_idx_;  // 读取起始位置（闭）
        uint64_t write_idx_; // 写入起始位置（闭）
    };
}

#endif