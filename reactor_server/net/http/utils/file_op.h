#ifndef __rs_file_op_h__
#define __rs_file_op_h__

#include <fstream>
#include <filesystem>
#include <reactor_server/base/log.h>

namespace rs_file_op
{
    using namespace log_system;

    class FileOp
    {
    public:
        // 读取文件
        static bool readFile(const std::filesystem::path &filepath, std::string &out_buf)
        {
            // 判断文件是否存在
            bool ret = std::filesystem::exists(filepath);
            if(!ret)
            {
                LOG(Level::Warning, "文件：{}打开失败", filepath.filename().string());
                return false;
            }

            // 打开文件，将内容写入到输出buffer中
            std::ifstream ifs(filepath, std::ios::binary);
            if(!ifs.is_open())
            {
                LOG(Level::Warning, "文件：{}打开失败", filepath.filename().string());
                return false;
            }
            // 获取文件大小开辟输出缓冲区
            out_buf.resize(std::filesystem::file_size(filepath));
            // 写入文件内容到输出缓冲区
            ifs.read(&out_buf[0], out_buf.size());
            ifs.close();
            
            return true;
        }

        // 写入文件
        static bool writeFile(const std::filesystem::path &filepath, const std::string &in_buf)
        {
            // 二进制+截断方式打开文件并写入
            std::ofstream ofs(filepath, std::ios::binary | std::ios::trunc);
            if(!ofs.is_open())
            {
                LOG(Level::Warning, "文件：{}打开失败", filepath.filename().string());
                return false;
            }
            // 写入文件内容
            // 不使用c_str，确保不写入\0
            ofs.write(in_buf.data(), in_buf.size());
            ofs.close();

            return true;
        }

        // 判断是否是目录
        static bool isDirectory(const std::filesystem::path &filepath)
        {
            return std::filesystem::is_directory(filepath);
        }

        // 判断是否是普通文件
        static bool isRegularFile(const std::filesystem::path &filepath)
        {
            return std::filesystem::is_regular_file(filepath);
        }
    };
}

#endif