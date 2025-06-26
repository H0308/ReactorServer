/*多条请求同时发送测试：一次性给服务器发送多条数据，然后查看服务器的处理结果，每一条请求都应该得到正常处理*/

#include <iostream>
#include <cassert>
#include <reactor_server/net/socket.h>
#include <reactor_server/base/log.h>

using namespace log_system;

int main()
{
    rs_socket::Socket cli_sock;
    cli_sock.createClient("127.0.0.1", 8080);
    std::string req = "GET /get HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 4\r\n\r\ntest";
    req += "GET /get HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 4\r\n\r\ntest";
    req += "GET /get HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 4\r\n\r\ntest";
    while (1)
    {
        assert(cli_sock.send_block(req.c_str(), req.size()) != -1);
        char buf[1024] = {0};
        assert(cli_sock.recv_block(buf, 1023));
        std::string str(buf);
        LOG(Level::Debug, "客户端收到数据：[{}]", str);
        LOG(Level::Debug, "进行下一次请求");
        sleep(3);
    }
    cli_sock.close();
    return 0;
}