#include <iostream>
#include <reactor_server/net/http/utils/info_get.h>
#include <reactor_server/net/http/utils/file_op.h>

int main()
{
    const std::string file = "a";
    std::string ext = rs_file_op::FileOp::getExtensionName(file);

    std::cout << rs_info_get::InfoGet::getMimeType(ext);

    return 0;
}