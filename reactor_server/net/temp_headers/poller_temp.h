#ifndef __rs_poller_temp_h__
#define __rs_poller_temp_h__

#include <sys/epoll.h>
#include <array>
#include <unordered_map>
#include <vector>
#include <reactor_server/base/log.h>
#include <reactor_server/base/error.h>
#include <reactor_server/net/temp_headers/channel_temp.h>

namespace rs_poller
{
    using namespace rs_log_system;

    const int max_ready_events = 1024;

    // 对epoll操作的封装以及上层使用简化
    class Poller
    {
    public:
        using ptr = std::shared_ptr<Poller>;

        Poller()
        {
            epfd_ = epoll_create(256);
            if(epfd_ < 0)
            {
                LOG(Level::Error, "创建Epoll模型失败");
                exit(static_cast<int>(rs_error::ErrorNum::Epoll_create_fail));
            }
        }

        // 添加/更新指定描述符的事件监控
        void updateEvent(rs_channel::Channel::ptr channel)
        {
            // 存在就更新，不存在就添加
            int fd = channel->getFd();
            auto pos = channels_.try_emplace(fd, channel);
            if(pos.second)
                update(EPOLL_CTL_ADD, channel);
            else
                update(EPOLL_CTL_MOD, channel);
        }

        // 移除指定描述符的事件监控
        void removeEvent(rs_channel::Channel::ptr channel)
        {
            auto it = channels_.find(channel->getFd());
            if(it == channels_.end())
                return;
            update(EPOLL_CTL_DEL, channel);
            // 从管理结构中移除对应的事件监控对象
            channels_.erase(it);
        }

        // 开启监控并获取就绪数组
        int startEpoll(std::vector<rs_channel::Channel::ptr> &channels)
        {
            // 阻塞等待
            int nfds = epoll_wait(epfd_, epoll_events_.data(), max_ready_events, -1);
            if(nfds < 0)
            {
                // 被中断打断，属于可接受范围
                if(errno == EINTR)
                    return 0;
                LOG(Level::Error, "事件等待失败：{}", strerror(errno));
                exit(static_cast<int>(rs_error::ErrorNum::Epoll_wait_fail));
            }

            // 等待成功将就绪的事件监控结构返回
            for(int i = 0; i < nfds; i++)
            {
                // 判断指定文件描述符是否存在
                auto it = channels_.find(epoll_events_[i].data.fd);
                assert(it != channels_.end());
                // 存在再设置对应的就绪事件
                auto channel = it->second;
                channel->setReadyEvents(epoll_events_[i].events);
                channels.emplace_back(channel);
            }

            return nfds;
        }

    private:
        // 直接进行epoll_ctl的操作封装
        void update(int op, rs_channel::Channel::ptr channel)
        {
            int fd = channel->getFd();
            struct epoll_event ev;
            ev.data.fd = fd;
            ev.events = channel->getEvents();
            int ret = epoll_ctl(epfd_, op, fd, &ev);
            if(ret < 0)
            {
                LOG(Level::Error, "添加文件描述符监控失败");
                exit(static_cast<int>(rs_error::ErrorNum::Epoll_ctl_fail));
            }
        }

    private:
        int epfd_;                                                     // epoll文件描述符
        std::array<struct epoll_event, max_ready_events> epoll_events_; // 就绪事件数组
        std::unordered_map<int, rs_channel::Channel::ptr> channels_;    // 管理的事件监控结构
    };
}

// 确保先生成Poller模块再使用其中的方法，在Channel模块中的前向声明只能声明类型，但是不能调用其中的方法
void rs_channel::Channel::remove()
{
    poller_->removeEvent(shared_from_this());
}

void rs_channel::Channel::update()
{
    poller_->updateEvent(shared_from_this());
}

#endif