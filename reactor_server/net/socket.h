#ifndef __rs_socket_h__
#define __rs_socket_h__

#include <cstdint>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <reactor_server/base/log.h>

namespace rs_socket
{
    using namespace log_system;

    const uint16_t default_port = 8080;
    const std::string default_ip = "127.0.0.1";

    // 默认连接队列大小
    const int default_backlog = 1024;

    class Socket
    {
    public:
        using ptr = std::shared_ptr<Socket>;

        // 委托构造
        Socket()
            : Socket(-1)
        {
        }

        Socket(int fd)
            : sockfd_(fd)
        {
        }

        // 创建套接字
        bool socket()
        {
            sockfd_ = ::socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd_ < 0)
            {
                LOG(Level::Error, "创建套接字失败");
                return false;
            }

            return true;
        }

        // 绑定地址信息
        bool bind(uint64_t port = default_port)
        {
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = INADDR_ANY;

            int ret = ::bind(sockfd_, reinterpret_cast<const struct sockaddr *>(&addr), sizeof(addr));

            if (ret < 0)
            {
                LOG(Level::Error, "绑定失败：{}", strerror(errno));
                return false;
            }

            return true;
        }

        // 开始监听
        bool listen(int backlog = default_backlog)
        {
            int ret = ::listen(sockfd_, backlog);
            if (ret < 0)
            {
                LOG(Level::Error, "监听错误");
                return false;
            }

            return true;
        }

        // 获取客户端连接
        int accept()
        {
            struct sockaddr_in addr;
            socklen_t len = sizeof(addr);

            int newfd = ::accept(sockfd_, reinterpret_cast<struct sockaddr *>(&addr), &len);

            if (newfd < 0)
            {
                LOG(Level::Warning, "获取客户端连接失败");
                return -1;
            }

            return newfd;
        }

        // 客户端发起连接
        bool connect(const std::string &ip = default_ip, uint16_t port = default_port)
        {
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = inet_addr(ip.c_str());

            int ret = ::connect(sockfd_, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
            if (ret < 0)
            {
                LOG(Level::Error, "连接服务端失败");
                return false;
            }

            return true;
        }

        // 发送
        ssize_t send_block(const void *buf, size_t len, int flag = 0)
        {
            if(len == 0)
                return 0;
            ssize_t ret = send(sockfd_, buf, len, flag);
            if(ret < 0)
            {
                if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                    return 0;
                LOG(Level::Error, "发送失败");
                return -1;
            }

            return ret;
        }

        // 发送
        ssize_t send_nonBlock(void *buf, size_t len)
        {
            return send_block(buf, len, MSG_DONTWAIT);
        }

        // 接收
        ssize_t recv_block(void *buf, size_t len, int flag = 0)
        {
            ssize_t ret = recv(sockfd_, buf, len, flag);
            if (ret < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                    return 0;
                LOG(Level::Error, "接收失败");
                return -1;
            }
            else if (ret == 0)
            {
                // 对端正常关闭连接
                LOG(Level::Info, "客户端断开连接");
                return -1;
            }

            return ret;
        }

        ssize_t recv_nonBlock(void *buf, size_t len)
        {
            return recv_block(buf, len, MSG_DONTWAIT);
        }

        // 关闭套接字
        void close()
        {
            if(sockfd_ > 0)
                ::close(sockfd_);
        }

        // 创建一个客户端
        bool createClient(const std::string &ip = default_ip, uint16_t port = default_port)
        {
            if(!socket())
                return false;
            if(!connect(ip, port))
                return false;

            return true;
        }

        // 创建一个服务端
        bool createServer(uint16_t port = default_port, bool isNonBlock = false)
        {
            if(!socket())
                return false;
            if(isNonBlock)
                setSocketNonBlock();
            if(!bind(port))
                return false;
            if(!listen())
                return false;

            setReuseAddressAndPort();

            return true;
        }

        // 开启地址重用
        void setReuseAddressAndPort()
        {
            int val = 1;
            setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, (void *)&val, sizeof(int));
            val = 1;
            setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, (void *)&val, sizeof(int));
        }

        // 开启套接字非阻塞
        void setSocketNonBlock()
        {
            int flag = fcntl(sockfd_, F_GETFL, 0);
            fcntl(sockfd_, F_SETFL, flag | O_NONBLOCK);
        }

        // 获取监听套接字
        int getSockFd()
        {
            return sockfd_;
        }

        ~Socket()
        {
            close();
        }

    private:
        int sockfd_;
    };
}

#endif