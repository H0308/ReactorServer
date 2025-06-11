#ifndef __rs_error_h__
#define __rs_error_h__

namespace rs_error
{
    enum class ErrorNum
    {
        Epoll_create_fail, // Epoll模型创建失败
        Epoll_ctl_fail, // Epoll操作失败
        Epoll_wait_fail, // Epoll监控失败
        EventFd_create_fail, // 事件监控文件描述符创建失败
        EventFd_read_fail, // 事件监控文件描述符读取失败
        EventFd_write_fail, // 事件监控文件描述符写入失败
        Timerfd_create_fail, // 定时器文件描述符创建失败
        Timerfd_read_fail // 定时器文件描述符读取失败
    };
}

#endif