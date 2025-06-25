#ifndef __rs_common_op_h__
#define __rs_common_op_h__

#include <string>
#include <vector>
#include <string_view>
#include <reactor_server/base/log.h>

namespace rs_common_op
{
    using namespace log_system;

    class CommonOp
    {
    public:
        // 字符串分割
        // 使用string_view自主实现split
        static size_t split(std::vector<std::string> &out, std::string_view line, std::string_view sep)
        {
            if (line.empty())
                return 0;

            size_t pos = 0;
            size_t found = 0;

            while ((found = line.find(sep, pos)) != std::string_view::npos)
            {
                std::string_view single = line.substr(pos, found - pos);
                // 越过没有内容的子串
                if(single.size() == 0)
                {
                    pos = found + sep.length();
                    continue;
                }
                // 添加当前位置到分隔符之间的子字符串
                out.push_back(std::string(single));
                // 更新位置到分隔符之后
                pos = found + sep.length();
            }

            // 添加最后一个分隔符之后的子字符串
            if (pos < line.length())
                out.push_back(std::string(line.substr(pos)));

            return out.size();
        }

        // 资源有效路径检查
        // 遇到前往上一级的标记就对目录层级进行减一，否则加一，任意一次目录层级减少到负数就返回假
        // 否则返回真
        static bool isValidResourcePath(const std::string &path)
        {
            if(path.size() == 0)
            {
                LOG(Level::Warning, "资源路径为空，检查失败");
                return false;
            }
            // 指定根目录直接返回
            if(path == "/")
                return true;

            // 分割路径
            std::vector<std::string> out;
            std::string sep = "/";
            bool ret = split(out, path, sep);
            if(!ret)
            {
                LOG(Level::Warning, "资源路径检查失败，路径分割失败");
                return false;
            }

            int level = 0;

            for(auto &str : out)
            {
                if(str == "..")
                {
                    level--;
                    if (level < 0)
                    {
                        LOG(Level::Warning, "资源路径无效");
                        return false;
                    }

                    continue;
                }

                level++;
            }

            return true;
        }
    };
}

#endif