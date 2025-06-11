#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <algorithm>
#include <reactor_server/net/socket.h>
#include <reactor_server/net/channel.h>
#include <reactor_server/net/event_loop_lock_queue.h>
#include <reactor_server/base/log.h>

using namespace log_system;

void handleRead(const rs_channel::Channel::ptr &client_channel, const rs_socket::Socket::ptr &client_socket);
void handleWrite(const rs_channel::Channel::ptr &client_channel, const rs_socket::Socket::ptr &client_socket);
void handleClose(const rs_channel::Channel::ptr &client_channel);

void handleAccept(const rs_channel::Channel::ptr &channel, const rs_socket::Socket::ptr &socket, rs_event_loop_lock_queue::EventLoopLockQueue* loop)
{
    // 获取新连接，并设置新连接的回调函数
    int newfd = socket->accept();
    // 创建客户端套接字结构
    rs_socket::Socket::ptr client_socket = std::make_shared<rs_socket::Socket>(newfd);
    rs_channel::Channel::ptr client_channel = std::make_shared<rs_channel::Channel>(loop, newfd);
    client_channel->setReadCallback(std::bind(&handleRead, client_channel, client_socket));
    client_channel->setWriteCallback(std::bind(&handleWrite, client_channel, client_socket));
    client_channel->setCloseCallback(std::bind(&handleClose, client_channel));
    // 对客户端数据文件描述符启用读事件监控
    client_channel->enableConcerningReadFd();
}

void handleRead(const rs_channel::Channel::ptr &client_channel, const rs_socket::Socket::ptr &client_socket)
{
    char *buffer[1024] = {0};
    // 读取客户端发送的数据
    ssize_t n = client_socket->recv_block(buffer, sizeof(buffer) - 1);
    if (n > 0)
    {
        std::string temp_buffer(reinterpret_cast<const char *>(buffer));
        // 关闭客户端连接
        LOG(Level::Debug, "收到客户端消息：{}", temp_buffer);
        // 触发可写事件
        client_channel->enableConcerningWriteFd();
    }
    else
    {
        handleClose(client_channel);
    }
}

void handleWrite(const rs_channel::Channel::ptr &client_channel, const rs_socket::Socket::ptr &client_socket)
{
    // 向客户端响应数据
    const std::string data = "服务端收到客户端发送的数据";
    ssize_t n = client_socket->send_block(data.c_str(), data.size());
    if (n < 0)
    {
        LOG(Level::Debug, "服务端数据发送失败");
        handleClose(client_channel);
        return;
    }

    // 关闭写事件关心
    client_channel->disableConcerningWriteFd();
}

void handleClose(const rs_channel::Channel::ptr &client_channel)
{
    // 取消当前文件描述符所有事件监控
    client_channel->disableConcerningAll();
    // 移除文件描述符
    client_channel->removeFd();
}

void testBasicServerOperations()
{
    std::cout << "=== 测试基本服务器操作 ===" << std::endl;

    rs_socket::Socket::ptr server = std::make_shared<rs_socket::Socket>();

    // 测试创建服务器
    bool result = server->createServer(8080, false);
    if (!result)
    {
        std::cout << "❌ 创建服务器失败" << std::endl;
        return;
    }
    std::cout << "✅ 服务器创建成功，监听端口 8080" << std::endl;

    std::cout << "等待客户端连接..." << std::endl;

    // 创建EventLoop进行对监听套接字进行监控
    rs_event_loop_lock_queue::EventLoopLockQueue::EventLoopLockQueue::ptr loop = std::make_shared<rs_event_loop_lock_queue::EventLoopLockQueue>();
    rs_channel::Channel::ptr listen_channel = std::make_shared<rs_channel::Channel>(loop.get(), server->getSockFd());
    // 设置监听套接字读事件的回调并启用监听套接字的读事件
    listen_channel->setReadCallback(std::bind(&handleAccept, listen_channel, server, loop.get()));
    listen_channel->enableConcerningReadFd();

    loop->startEventLoop();

    // 等待一段时间再关闭
    sleep(1);
    std::cout << "服务器测试完成" << std::endl;
}

int main()
{
    std::cout << "开始服务器Socket功能测试...\n"
              << std::endl;

    testBasicServerOperations();

    std::cout << "\n🎉 服务器测试完成！" << std::endl;
    return 0;
}