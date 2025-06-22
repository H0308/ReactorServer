#include <iostream>
#include <reactor_server/net/http/utils/file_op.h>

int main()
{
    const std::string filepath = "/home/epsda/ReactorServer/reactor_server/net/channel.h";
    std::string buf;
    bool ret = rs_file_op::FileOp::readFile(filepath, buf);
    if(ret)
        std::cout << buf;

    std::cout << "=========\n";

    const std::string outPath = "./test.h";
    ret = rs_file_op::FileOp::writeFile(outPath, buf);
    if(ret)
        std::cout << "写入文件完成" << std::endl;

    return 0;
}