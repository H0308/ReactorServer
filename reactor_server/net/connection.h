#ifndef __rs_connection_h__
#define __rs_connection_h__

#include <any>
#include <reactor_server/base/log.h>
#include <reactor_server/net/buffer.h>
#include <reactor_server/net/socket.h>
#include <reactor_server/net/event_loop_lock_queue.h>

namespace rs_connection
{
    // 连接状态
    enum class ConnectionStatus
    {
        Disconnected, // 连接断开完毕
        Disconnecting, // 连接断开中
        Connected, // 连接建立完成
        Connecting // 连接建立中
    };

    class Connection : public std::enable_shared_from_this<Connection>
    {
    public:
        using ptr = std::shared_ptr<Connection>;
        // 连接建立完成回调
        using connectedCallback_t = std::function<void(const Connection::ptr &)>;
        // 消息处理回调
        using messageCallback_t = std::function<void(const Connection::ptr &, rs_buffer::Buffer &)>;
        // 连接处理回调
        using closeCallback_t = std::function<void(const Connection::ptr &)>;
        // 任意事件回调
        using anyEventCallback_t = std::function<void(const Connection::ptr &)>;

        void establishAfterConnected()
        {

        }

        void send(void *data, size_t len)
        {

        }

        void shutdown()
        {

        }

        void enableTimeoutRelease(uint32_t timeout)
        {
            
        }

        void disableTimeoutRelease()
        {

        }

        void switchProtocol(const std::any &context, const connectedCallback_t &con_cb, const messageCallback_t &msg_cb, const closeCallback_t &close_cb, const anyEventCallback_t &any_cb)
        {

        }

        void setConnectedCallback(const connectedCallback_t &cb)
        {

        }

        void setMessageCallback(const messageCallback_t &cb)
        {

        }

        void setCloseCallback(const closeCallback_t &cb)
        {

        }

        void setAnyEventCallback(const anyEventCallback_t &cb)
        {

        }

        int getFd()
        {
            return fd_;
        }

        std::any &getContext()
        {
            return context_;
        }

        void setContext(const std::any &context)
        {
        }

    private:
        void establishAfterConnectedInLoop()
        {
        }

        void sendInLoop(void *data, size_t len)
        {
        }

        void release()
        {

        }

        void shutdownInLoop()
        {
        }

        void enableTimeoutReleaseInLoop(uint32_t timeout)
        {
        }

        void disableTimeoutReleaseInLoop()
        {
        }

        void switchProtocolInLoop(const std::any &context, const connectedCallback_t &con_cb, const messageCallback_t &msg_cb, const closeCallback_t &close_cb, const anyEventCallback_t &any_cb)
        {
        }

        // 提供给Channel模块的回调函数
        void handleRead()
        {
            char buffer[65536] = {0};
            // 读取数据并放入到输入缓冲区中
            // 再将输入缓冲区中的数据交给消息回调处理
            ssize_t ret = socket_->recv_nonBlock(buffer, sizeof(buffer) - 1);
            if(ret < 0)
            {
                // 释放资源后关闭连接
                shutdownInLoop();
                return;
            }

            // 写入数据到输入缓冲区
            // 读取为0依旧当做有数据处理，只是写入的数据大小为0
            in_buffer_.write_move(buffer, sizeof(buffer));
            if(in_buffer_.getReadableSize() > 0)
                if(msg_cb_)
                    msg_cb_(shared_from_this(), in_buffer_);
        }

        void handleWrite()
        {
            // 将输出缓冲区中的数据进行发送
            ssize_t ret = socket_->send_nonBlock(out_buffer_.getReadPos(), out_buffer_.getReadableSize());
            if(ret < 0)
            {
                // 判断输入缓冲区是否还有数据需要处理
                if (in_buffer_.getReadableSize() > 0)
                    if (msg_cb_)
                        msg_cb_(shared_from_this(), in_buffer_);
                // 处理完毕后直接释放连接
                release();
            }
            // 移动读指针
            out_buffer_.moveReadPtr(ret);
            // 如果可读空间为0，说明数据已经全部发送完毕，关闭可读事件监控防止持续触发可读事件
            if(out_buffer_.getReadableSize() == 0)
            {
                channel_->disableConcerningWriteFd();
                // 如果连接状态为待关闭，则释放连接
                if(con_status_ == ConnectionStatus::Disconnecting)
                    release();
            }
        }

        void handleClose()
        {
            // 判断输入缓冲区是否还有数据需要处理
            if (in_buffer_.getReadableSize() > 0)
                if (msg_cb_)
                    msg_cb_(shared_from_this(), in_buffer_);
            // 处理完毕后直接释放连接
            release();
        }

        void handleError()
        {
            handleClose();
        }

        void handleAny()
        {
            // 判断是否启用连接超时释放
            if (enable_timeout_release_)
                event_loop_->refreshTask(id_);

            // 调用任意事件回调
            if(any_cb_)
                any_cb_(shared_from_this());
        }

    private:
        std::string id_; // 连接ID，同时也是定时任务ID
        int fd_; // 管理的文件描述符
        rs_socket::Socket::ptr socket_; // 套接字管理结构
        rs_channel::Channel::ptr channel_; // 事件管理模块
        rs_event_loop_lock_queue::EventLoopLockQueue::ptr event_loop_; // 事件监控模块
        rs_buffer::Buffer in_buffer_; // 输入缓冲区
        rs_buffer::Buffer out_buffer_; // 输出缓冲区
        std::any context_; // 协议上下文管理
        ConnectionStatus con_status_; // 连接状态
        bool enable_timeout_release_; // 连接超时释放标记

        connectedCallback_t con_cb_;
        messageCallback_t msg_cb_;
        closeCallback_t outer_close_cb_;
        anyEventCallback_t any_cb_;

        closeCallback_t inner_close_cb_; // 提供给服务器内部进行资源释放使用的关闭回调
    };
}

#endif