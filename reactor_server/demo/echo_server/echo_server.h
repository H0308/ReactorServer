#pragma once

#include <reactor_server/base/log.h>
#include <reactor_server/net/tcp_server.h>

namespace echo_server
{
    using namespace log_system;

    class EchoServer
    {
    public:
        EchoServer(int port)
            :server_(port)
        {
            server_.setConnectedCallback(std::bind(&EchoServer::onConnected, this, std::placeholders::_1));
            server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2));
            server_.setOuterCloseCallback(std::bind(&EchoServer::onClose, this, std::placeholders::_1));
            server_.enableTimeoutRelease(10);
            server_.setThreadNum(5);
        }

        void start()
        {
            server_.start();
        }

    private:
        void onConnected(const rs_connection::Connection::ptr &con)
        {
            // 打印客户端信息
            LOG(Level::Debug, "客户端：{} 连接", con->getFd());
        }

        void onMessage(const rs_connection::Connection::ptr &con, rs_buffer::Buffer &buf)
        {
            // 输出接收到的信息并向客户端返回数据
            std::string data(reinterpret_cast<const char *>(buf.getReadPos()), buf.getReadableSize());
            // LOG(Level::Debug, "服务端收到客户端消息：{}", data);
            // printf("客户端发送：%s", buf.getReadPos());
            buf.moveReadPtr(buf.getReadableSize());

            con->send((void *)(data.c_str()), data.size());
        }

        void onClose(const rs_connection::Connection::ptr &con)
        {
            LOG(Level::Debug, "客户端：{} 断开连接", con->getFd());
        }

    private:
        rs_tcp_server::TcpServer server_;
    };
}