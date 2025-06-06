#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <reactor_server/net/socket.h>

using namespace rs_socket;

void testBasicServerOperations()
{
    std::cout << "=== 测试基本服务器操作 ===" << std::endl;

    Socket server;

    // 测试创建服务器
    bool result = server.createServer(8080, false);
    if (!result)
    {
        std::cout << "❌ 创建服务器失败" << std::endl;
        return;
    }
    std::cout << "✅ 服务器创建成功，监听端口 8080" << std::endl;

    std::cout << "等待客户端连接..." << std::endl;

    // 接受客户端连接
    int client_fd = server.accept();
    if (client_fd < 0)
    {
        std::cout << "❌ 接受客户端连接失败" << std::endl;
        return;
    }
    std::cout << "✅ 客户端连接成功，fd: " << client_fd << std::endl;

    // 创建用于通信的Socket对象
    Socket client_socket(client_fd);

    // 接收客户端数据
    char buffer[1024] = {0};
    ssize_t bytes_received = client_socket.recv_block(buffer, sizeof(buffer) - 1);
    if (bytes_received > 0)
    {
        buffer[bytes_received] = '\0';
        std::cout << "✅ 接收到客户端数据: " << buffer << std::endl;

        // 回复客户端
        std::string response = "Server received: " + std::string(buffer);
        ssize_t bytes_sent = client_socket.send_block((void *)response.c_str(), response.length());
        if (bytes_sent > 0)
        {
            std::cout << "✅ 已回复客户端: " << response << std::endl;
        }
        else
        {
            std::cout << "❌ 发送回复失败" << std::endl;
        }
    }
    else
    {
        std::cout << "❌ 接收客户端数据失败" << std::endl;
    }

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