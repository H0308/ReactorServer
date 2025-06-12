#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <algorithm>
#include <reactor_server/base/log.h>
#include <reactor_server/base/uuid_generator.h>
#include <reactor_server/net/socket.h>
#include <reactor_server/net/channel.h>
#include <reactor_server/net/event_loop_lock_queue.h>
#include <reactor_server/net/connection.h>

using namespace log_system;

// 管理所有客户端连接的哈希表
std::unordered_map<std::string, rs_connection::Connection::ptr> clients;

void onConnected(const rs_connection::Connection::ptr &con);
void onMessage(const rs_connection::Connection::ptr &con, rs_buffer::Buffer &buf);
void onClose(const rs_connection::Connection::ptr &con);

void handleAccept(const rs_socket::Socket::ptr &socket, rs_event_loop_lock_queue::EventLoopLockQueue::ptr loop)
{
    // 获取新连接，并设置新连接的回调函数
    int newfd = socket->accept();
    // 创建客户端套接字结构
    const std::string id = rs_uuid_generator::UuidGenerator::generate_uuid();
    rs_connection::Connection::ptr client = std::make_shared<rs_connection::Connection>(loop, id, newfd);

    client->enableTimeoutRelease(10);

    client->setConnectedCallback(std::bind(onConnected, std::placeholders::_1));
    client->setMessageCallback(std::bind(onMessage, std::placeholders::_1, std::placeholders::_2));
    client->setInnerCloseCallback(std::bind(onClose, std::placeholders::_1));
    client->establishAfterConnected();

    // 管理连接的客户端
    clients.try_emplace(id, client);
}

void onConnected(const rs_connection::Connection::ptr &con)
{
    // 打印客户端信息
    LOG(Level::Debug, "客户端：{} 连接", con->getFd());
}

void onMessage(const rs_connection::Connection::ptr &con, rs_buffer::Buffer &buf)
{
    // 输出接收到的信息并向客户端返回数据
    // const std::string data(buf.getReadPos(), buf.getReadableSize());
    // LOG(Level::Debug, "服务端收到客户端消息：{}", data);
    printf("客户端发送：%s", buf.getReadPos());
    buf.moveReadPtr(buf.getReadableSize());

    // 服务端返回响应
    const std::string data1 = "hello world";
    con->send((void*)(data1.c_str()), data1.size());
}

void onClose(const rs_connection::Connection::ptr &con)
{
    LOG(Level::Debug, "客户端：{} 断开连接", con->getFd());
    clients.erase(con->getId());
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
    rs_event_loop_lock_queue::EventLoopLockQueue::ptr loop = std::make_shared<rs_event_loop_lock_queue::EventLoopLockQueue>();
    rs_channel::Channel::ptr listen_channel = std::make_shared<rs_channel::Channel>(loop.get(), server->getSockFd());
    // 设置监听套接字读事件的回调并启用监听套接字的读事件
    listen_channel->setReadCallback(std::bind(&handleAccept, server, loop));
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