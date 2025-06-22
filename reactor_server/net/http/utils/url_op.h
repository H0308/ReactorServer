#ifndef __rs_url_op_h__
#define __rs_url_op_h__

#include <string>
#include <cctype>

namespace rs_url_op
{
    class UrlOp
    {
    public:
        // 对URL进行编码
        // 除了.-_~或者字母和数字不需要编码外，其他均需要编码
        // 对于空格来说，取决于标准，根据W3C标准规定空格需要转换为+，本次默认不编码，可选编码
        static bool urlEncode(std::string &out, const std::string &in, bool convert_space = false)
        {
            if(in.size() == 0)
                return false;

            // 遍历输入字符串进行追加
            for(auto &ch : in)
            {
                if(ch == '.' || ch == '-' || ch == '_' || ch == '~' || isalnum(ch))
                {
                    out += ch;
                    continue;
                }
                if (ch == ' ' && convert_space)
                {
                    out += '+';
                    continue;
                }

                // 其余字符全部编码为%+两位16进制值，字母大写
                char temp[4] = {0};
                snprintf(temp, 4, "%%%02X", ch);
                out += temp;
            }

            return true;
        }

        // 对URL进行解码
        static bool urlDecode(std::string &out, const std::string &in, bool convert_space = false)
        {
            if (in.size() == 0)
                return false;

            for (size_t i = 0; i < in.size(); i++)
            {
                if(in[i] == '+' && convert_space)
                {
                    out += ' ';
                    continue;
                }

                // 对已经编码的字符进行转换
                if(in[i] == '%' && i + 2 < in.size())
                {
                    char num1 = hexToInt(in[i + 1]);
                    char num2 = hexToInt(in[i + 2]);
                    char sum = (num1 << 4) + num2;
                    out += sum;
                    i += 2;
                    continue;
                }

                out += in[i];
            }
            
            return true;
        }

    private:
        static char hexToInt(char c)
        {
            if (isdigit(c))
                return c - '0';
            else if (islower(c))
                return c - 'a' + 10;
            else if (isupper(c))
                return c - 'A' + 10;

            return -1;
        }
    };
}

#endif