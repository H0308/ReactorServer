#include <iostream>
#include <reactor_server/net/http/utils/common_op.h>

int main()
{
    std::string line = "abc";
    std::string sep = ",";
    std::vector<std::string> out;

    size_t ret = rs_common_op::CommonOp::split(out, line, sep);

    for(auto &str : out)
        std::cout << str << std::endl;

    return 0;
}