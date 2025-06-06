#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <reactor_server/net/socket.h>

using namespace rs_socket;

void testBasicClientOperations()
{
    std::cout << "=== 测试基本客户端操作 ===" << std::endl;

    // 等待服务器启动
    sleep(1);

    Socket client;

    // 测试创建客户端并连接
    bool result = client.createClient("127.0.0.1", 8080);
    if (!result)
    {
        std::cout << "❌ 连接服务器失败" << std::endl;
        return;
    }
    std::cout << "✅ 连接服务器成功" << std::endl;

    // 发送数据到服务器
    std::string message = "Hello from client!";
    ssize_t bytes_sent = client.send_block((void *)message.c_str(), message.length());
    if (bytes_sent > 0)
    {
        std::cout << "✅ 发送数据成功: " << message << std::endl;
    }
    else
    {
        std::cout << "❌ 发送数据失败" << std::endl;
        return;
    }

    // 接收服务器回复
    char buffer[1024] = {0};
    ssize_t bytes_received = client.recv_block(buffer, sizeof(buffer) - 1);
    if (bytes_received > 0)
    {
        buffer[bytes_received] = '\0';
        std::cout << "✅ 接收到服务器回复: " << buffer << std::endl;
    }
    else
    {
        std::cout << "❌ 接收服务器回复失败" << std::endl;
    }

    std::cout << "客户端基本操作测试完成" << std::endl;
}

int main()
{
    std::cout << "开始客户端Socket功能测试...\n"
              << std::endl;

    testBasicClientOperations();

    std::cout << "\n🎉 客户端测试完成！" << std::endl;
    return 0;
}