#ifndef __rs_error_h__
#define __rs_error_h__

namespace rs_error
{
    enum class ErrorNum
    {
        Epoll_create_fail,
        Epoll_ctl_fail,
        Epoll_wait_fail
    };
}

#endif