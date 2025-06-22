#include <iostream>
#include <reactor_server/net/http/utils/common_op.h>

int main()
{
    std::string path = "/html/../login.html";
    bool ret = rs_common_op::CommonOp::isValidResourcePath(path);
    std::cout << std::boolalpha << ret << std::endl;

    return 0;
}