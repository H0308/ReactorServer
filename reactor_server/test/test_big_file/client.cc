/*大文件传输测试，给服务器上传一个大文件，服务器将文件保存下来，观察处理结果。上传的文件必须和服务器保存的文件一致*/
// 操作：使用dd if=/dev/zero of=./hello.txt bs=100MB count=3在客户端测试目录下创建一个300MB的文件发送给服务端
// 服务端接收该文件并存储

#include <iostream>
#include <cassert>
#include <reactor_server/net/socket.h>
#include <reactor_server/base/log.h>
#include <reactor_server/net/http/utils/file_op.h>

using namespace rs_log_system;

int main()
{
    rs_socket::Socket cli_sock;
    cli_sock.createClient("127.0.0.1", 8080);
    std::string req = "PUT /put HTTP/1.1\r\nConnection: keep-alive\r\n";
    std::string body;
    rs_file_op::FileOp::readFile("./hello.txt", body);
    req += "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n";
    assert(cli_sock.send_block(req.c_str(), req.size()) != -1);
    assert(cli_sock.send_block(body.c_str(), body.size()) != -1);
    char buf[1024] = {0};
    assert(cli_sock.recv_block(buf, 1023));
    std::string str(buf);
    LOG(Level::Debug, "客户端接收到数据：[{}]", str);
    sleep(3);
    cli_sock.close();
    return 0;
}