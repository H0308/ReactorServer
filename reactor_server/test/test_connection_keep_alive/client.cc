/*长连接测试1：创建一个客户端持续给服务器发送数据，直到超过超时时间看看是否正常*/
#include <iostream>
#include <cassert>
#include <reactor_server/net/socket.h>
#include <reactor_server/base/log.h>

using namespace log_system;

int main()
{
    rs_socket::Socket cli_sock;
    cli_sock.createClient("127.0.0.1", 8080);
    std::string req = "GET /get HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
    while (1)
    {
        assert(cli_sock.send_block(req.c_str(), req.size()) != -1);
        // char buf[1024] = {0};
        // assert(cli_sock.recv_block(buf, 1023));
        LOG(Level::Debug, "本次发送成功，进入下一轮发送");
        sleep(3);
    }
    cli_sock.close();
    return 0;
}