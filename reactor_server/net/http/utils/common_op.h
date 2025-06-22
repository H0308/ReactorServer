#ifndef __rs_common_op_h__
#define __rs_common_op_h__

#include <string>
#include <vector>
#include <string_view>

namespace rs_common_op
{
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
    };
}

#endif