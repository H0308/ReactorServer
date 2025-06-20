#ifndef __rs_connection_h__
#define __rs_connection_h__

#include <any>
#include <reactor_server/base/log.h>
#include <reactor_server/net/buffer.h>
#include <reactor_server/net/socket.h>
#include <reactor_server/net/event_loop_lock_queue.h>

namespace rs_connection
{
    using namespace log_system;

    // 连接状态
    enum class ConnectionStatus
    {
        Disconnected,  // 连接断开完毕
        Disconnecting, // 连接断开中
        Connected,     // 连接建立完成
        Connecting     // 连接建立中
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

        Connection(rs_event_loop_lock_queue::EventLoopLockQueue *loop, const std::string &id, int fd)
            : fd_(fd), id_(id), event_loop_(loop), socket_(std::make_shared<rs_socket::Socket>(fd)), channel_(std::make_shared<rs_channel::Channel>(event_loop_, fd_)), con_status_(ConnectionStatus::Connecting), enable_timeout_release_(false)
        {
            // 设置回调给Channel，但是不启动读事件监控，确保定时任务可以正常使用
            // 防止出现定时任务没有启动之前有读事件发生，此时不存在定时任务导致错误刷新任务
            channel_->setReadCallback(std::bind(&Connection::handleRead, this));
            channel_->setWriteCallback(std::bind(&Connection::handleWrite, this));
            channel_->setCloseCallback(std::bind(&Connection::handleClose, this));
            channel_->setErrorCallback(std::bind(&Connection::handleError, this));
            channel_->setAnyCallback(std::bind(&Connection::handleAny, this));
        }

        void establishAfterConnected()
        {
            event_loop_->runTasks(std::bind(&Connection::establishAfterConnectedInLoop, this));
        }

        void send(void *data, size_t len)
        {
            /**
             * 因为runTasks可能只是将任务放入队列，并不是立即执行
             * 这就可能出现后续执行sendInLoop时，data数据已经被销毁
             * 此处构造一个新的buffer，在sendInLoop中使用参数的buffer再构造一个临时buffer用于数据转移到输出缓冲区
             */
            rs_buffer::Buffer buffer;
            buffer.write_move(data, len);
            event_loop_->runTasks(std::bind(&Connection::sendInLoop, this, buffer));
        }

        void shutdown()
        {
            event_loop_->runTasks(std::bind(&Connection::shutdownInLoop, this));
        }

        void enableTimeoutRelease(uint32_t timeout)
        {
            event_loop_->runTasks(std::bind(&Connection::enableTimeoutReleaseInLoop, this, timeout));
        }

        void disableTimeoutRelease()
        {
            event_loop_->runTasks(std::bind(&Connection::disableTimeoutReleaseInLoop, this));
        }

        void switchProtocol(const std::any &context, const connectedCallback_t &con_cb, const messageCallback_t &msg_cb, const closeCallback_t &close_cb, const anyEventCallback_t &any_cb)
        {
            event_loop_->assertInCurrentThread();
            // 必须保证协议切换是在同一线程下执行，防止后续使用新协议数据无法被正确解析
            event_loop_->runTasks(std::bind(&Connection::switchProtocolInLoop, this, context, con_cb, msg_cb, close_cb, any_cb));
        }

        void release()
        {
            event_loop_->runTasks(std::bind(&Connection::releaseInLoop, this));
        }

        void setConnectedCallback(const connectedCallback_t &cb)
        {
            con_cb_ = cb;
        }

        void setMessageCallback(const messageCallback_t &cb)
        {
            msg_cb_ = cb;
        }

        void setInnerCloseCallback(const closeCallback_t &cb)
        {
            inner_close_cb_ = cb;
        }

        void setOuterCloseCallback(const closeCallback_t &cb)
        {
            outer_close_cb_ = cb;
        }

        void setAnyEventCallback(const anyEventCallback_t &cb)
        {
            any_cb_ = cb;
        }

        int getFd()
        {
            return fd_;
        }

        const std::string getId()
        {
            return id_;
        }

        std::any &getContext()
        {
            return context_;
        }

        void setContext(const std::any &context)
        {
            context_ = context;
        }

    private:
        void establishAfterConnectedInLoop()
        {
            // 1. 更改连接状态由半连接到完全连接
            assert(con_status_ == ConnectionStatus::Connecting);
            con_status_ = ConnectionStatus::Connected;
            // 2. 启用文件描述符可读事件监控
            channel_->enableConcerningReadFd();
            // 3. 调用上层回调函数
            if (con_cb_)
                con_cb_(shared_from_this());
        }

        // 使用传值调用确保临时对象的创建
        void sendInLoop(rs_buffer::Buffer buffer)
        {
            // 该接口本身不发送数据，只是将数据放入写入缓冲区，启动读事件监控即可
            // 如果连接是待关闭状态就不再发送数据
            if (con_status_ == ConnectionStatus::Disconnected)
                return;
            out_buffer_.write_move(buffer);
            if (!channel_->checkIsConcerningWriteFd())
                channel_->enableConcerningWriteFd();
        }

        void releaseInLoop()
        {
            // 1. 更改连接状态为连接断开
            con_status_ = ConnectionStatus::Disconnected;
            // 2. 清空Channel的所有回调函数，防止悬空指针访问
            channel_->setReadCallback(nullptr);
            channel_->setWriteCallback(nullptr);
            channel_->setCloseCallback(nullptr);
            channel_->setErrorCallback(nullptr);
            channel_->setAnyCallback(nullptr);
            // 2. 关闭文件描述符事件监控并移除文件描述符
            channel_->disableConcerningAll();
            channel_->removeFd();
            // 3. 关闭描述符
            socket_->close();
            // 4. 移除定时任务
            if (enable_timeout_release_)
                if (event_loop_->hasTimer(id_))
                    disableTimeoutReleaseInLoop();
            // 5. 调用上层连接断开回调
            // 注意一定要先调用上层的回调，如果调用底层回调会因为释放连接结构导致上层野指针
            if (outer_close_cb_)
                outer_close_cb_(shared_from_this());
            // 6. 调用底层连接断开回调
            if (inner_close_cb_)
                inner_close_cb_(shared_from_this());
        }

        void shutdownInLoop()
        {
            // 1. 设置连接状态为半连接
            con_status_ = ConnectionStatus::Disconnecting;
            // 2. 如果输入缓冲区还有数据就调用上层回调进行处理
            if (in_buffer_.getReadableSize() > 0)
                msg_cb_(shared_from_this(), in_buffer_);
            // 3. 如果输出缓冲区有数据则启用写监控发送数据
            if (out_buffer_.getReadableSize() > 0)
                if (!channel_->checkIsConcerningWriteFd())
                    channel_->enableConcerningWriteFd();
            // 4. 释放连接
            release();
        }

        void enableTimeoutReleaseInLoop(uint32_t timeout)
        {
            // 更改标记位
            enable_timeout_release_ = true;
            // 判断是否存在对应的定时任务，存在即更新，不存在即添加
            if (event_loop_->hasTimer(id_))
                event_loop_->refreshTask(id_);
            else
                event_loop_->insertTask(id_, timeout, std::bind(&Connection::release, this));
        }

        void disableTimeoutReleaseInLoop()
        {
            // 更改标记位
            enable_timeout_release_ = false;
            event_loop_->cancelTask(id_);
        }

        void switchProtocolInLoop(const std::any &context, const connectedCallback_t &con_cb, const messageCallback_t &msg_cb, const closeCallback_t &close_cb, const anyEventCallback_t &any_cb)
        {
            context_ = context;
            con_cb_ = con_cb;
            msg_cb_ = msg_cb;
            outer_close_cb_ = close_cb;
            any_cb_ = any_cb;
        }

        // 提供给Channel模块的回调函数
        void handleRead()
        {
            // 保护当前对象生命周期，防止在处理过程中被释放
            auto self = shared_from_this();

            // 检查连接状态，如果已经断开则不处理
            if (con_status_ == ConnectionStatus::Disconnected ||
                con_status_ == ConnectionStatus::Disconnecting)
                return;

            char buffer[65536] = {0};
            // 读取数据并放入到输入缓冲区中
            // 再将输入缓冲区中的数据交给消息回调处理
            ssize_t ret = socket_->recv_nonBlock(buffer, 65535);
            if (ret < 0)
            {
                // 释放资源后关闭连接
                shutdownInLoop();
                return;
            }

            // debug
            // LOG(Level::Debug, "收到数据大小：{}", ret);

            // 写入数据到输入缓冲区
            // 读取为0依旧当做有数据处理，只是写入的数据大小为0
            in_buffer_.write_move(buffer, ret);
            if (in_buffer_.getReadableSize() > 0)
                if (msg_cb_)
                    msg_cb_(shared_from_this(), in_buffer_);
        }

        void handleWrite()
        {
            auto self = shared_from_this();

            if (con_status_ == ConnectionStatus::Disconnected)
                return;

            // 将输出缓冲区中的数据进行发送
            ssize_t ret = socket_->send_nonBlock(out_buffer_.getReadPos(), out_buffer_.getReadableSize());
            if (ret < 0)
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
            if (out_buffer_.getReadableSize() == 0)
            {
                channel_->disableConcerningWriteFd();
                // 如果连接状态为待关闭，则释放连接
                if (con_status_ == ConnectionStatus::Disconnecting)
                    release();
            }
        }

        void handleClose()
        {
            auto self = shared_from_this();

            // 判断输入缓冲区是否还有数据需要处理
            if (in_buffer_.getReadableSize() > 0)
                if (msg_cb_)
                    msg_cb_(shared_from_this(), in_buffer_);
            // 处理完毕后直接释放连接
            release();
        }

        void handleError()
        {
            auto self = shared_from_this();

            handleClose();
        }

        void handleAny()
        {
            // 检查对象是否已经被释放
            if (con_status_ == ConnectionStatus::Disconnected)
                return;

            auto self = shared_from_this();
            
            // 判断是否启用连接超时释放
            if (enable_timeout_release_)
                event_loop_->refreshTask(id_);

            // 调用任意事件回调
            if (any_cb_)
                any_cb_(shared_from_this());
        }

    private:
        std::string id_;                                           // 连接ID，同时也是定时任务ID
        int fd_;                                                   // 管理的文件描述符
        rs_socket::Socket::ptr socket_;                            // 套接字管理结构
        rs_event_loop_lock_queue::EventLoopLockQueue *event_loop_; // 事件监控模块
        rs_channel::Channel::ptr channel_;                         // 事件管理模块
        rs_buffer::Buffer in_buffer_;                              // 输入缓冲区
        rs_buffer::Buffer out_buffer_;                             // 输出缓冲区
        std::any context_;                                         // 协议上下文管理
        ConnectionStatus con_status_;                              // 连接状态
        bool enable_timeout_release_;                              // 连接超时释放标记

        connectedCallback_t con_cb_;
        messageCallback_t msg_cb_;
        closeCallback_t outer_close_cb_;
        anyEventCallback_t any_cb_;

        closeCallback_t inner_close_cb_; // 提供给服务器内部进行资源释放使用的关闭回调
    };
}

#endif