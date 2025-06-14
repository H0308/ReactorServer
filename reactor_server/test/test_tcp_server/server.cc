#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <algorithm>
#include <reactor_server/base/log.h>
#include <reactor_server/net/tcp_server.h>

using namespace log_system;

void onConnected(const rs_connection::Connection::ptr &con)
{
    // 打印客户端信息
    LOG(Level::Debug, "客户端：{} 连接", con->getFd());
}

void onMessage(const rs_connection::Connection::ptr &con, rs_buffer::Buffer &buf)
{
    std::cout << "当前线程为：" << pthread_self() << std::endl;

    // 输出接收到的信息并向客户端返回数据
    std::string data(reinterpret_cast<const char *>(buf.getReadPos()), buf.getReadableSize());
    LOG(Level::Debug, "服务端收到客户端消息：{}", data);
    // printf("客户端发送：%s", buf.getReadPos());
    buf.moveReadPtr(buf.getReadableSize());

    // 服务端返回响应
    const std::string data1 = "hello world";
    con->send((void *)(data1.c_str()), data1.size());
}

void onClose(const rs_connection::Connection::ptr &con)
{
    LOG(Level::Debug, "客户端：{} 断开连接", con->getFd());
}

int main()
{
    rs_tcp_server::TcpServer tcp_server(8080);
    tcp_server.setConnectedCallback(onConnected);
    tcp_server.setMessageCallback(onMessage);
    tcp_server.setOuterCloseCallback(onClose);
    tcp_server.enableTimeoutRelease(10);
    tcp_server.start();

    return 0;
}