#ifndef __rs_timing_wheel_h__
#define __rs_timing_wheel_h__

#include <vector>
#include <memory>
#include <sys/timerfd.h>
#include <unordered_map>
#include <reactor_server/base/log.h>
#include <reactor_server/base/error.h>
#include <reactor_server/net/schedule_task.h>
#include <reactor_server/net/channel.h>

namespace rs_event_loop_lock_queue
{
    class EventLoopLockQueue;
}

namespace rs_timing_wheel
{
    using namespace log_system;

    class TimingWheel
    {
    public:
        using ptr = std::shared_ptr<TimingWheel>;
        using per_task_ptr_t = std::shared_ptr<rs_schedule_task::ScheduleTask>;
        using per_task_ptr_t_weak = std::weak_ptr<rs_schedule_task::ScheduleTask>;

        TimingWheel(rs_event_loop_lock_queue::EventLoopLockQueue *loop)
            : capacity_(60), tick_(0), schedule_tasks_(capacity_), timerfd_(getTimerFd()), loop_(loop), timerfd_channel(std::make_shared<rs_channel::Channel>(loop_, timerfd_))
        {
            // 设置定时器文件描述符可读事件回调并启用可读事件监听
            timerfd_channel->setReadCallback(std::bind(&TimingWheel::executeTimerTask, this));
            timerfd_channel->enableConcerningReadFd();
        }

        // 将时间轮的任务全部交给EventLoop来处理，确保任务可以在一个线程内执行保证线程安全问题
        void cancelTask(const std::string &id);
        void insertTask(const std::string &id, uint32_t timeout, const rs_schedule_task::ScheduleTask::main_task_t &task);
        void refreshTask(const std::string &id);

        // 定时文件描述符可读事件触发回调
        void executeTimerTask()
        {
            int gap = readTimerFd();
            // 根据实际超时次数进行任务处理
            for(int i = 0; i < gap; i++)
                runTasks();
        }

        // 判断是否存在指定定时器
        // 非线程安全，使用时需要保证在同一线程内
        bool hasTimer(const std::string &id)
        {
            return static_cast<bool>(task_map_.count(id));
        }

    private:
        // 直接从哈希表中删除对应的任务
        void removeTask(std::string id)
        {
            auto pos = task_map_.find(id);
            if (pos == task_map_.end())
                return;

            task_map_.erase(id);
        }

        // 创建定时器文件描述符
        static int getTimerFd()
        {
            // 创建定时器描述符
            int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);

            if (timer_fd < 0)
            {
                LOG(Level::Error, "定时器文件描述符创建失败");
                exit(static_cast<int>(rs_error::ErrorNum::Timerfd_create_fail));
            }

            // 设置定时器
            struct itimerspec timer;
            // 初始时间
            timer.it_interval.tv_sec = 1;
            timer.it_interval.tv_nsec = 0; // 纳秒为0，防止随机值
            // 间隔时间
            timer.it_value.tv_sec = 1;
            timer.it_value.tv_nsec = 0;

            timerfd_settime(timer_fd, 0, &timer, NULL);

            return timer_fd;
        }

        // 执行任务函数，每秒执行1次的函数
        void runTasks()
        {
            tick_ = (tick_ + 1) % capacity_;
            // 销毁时间轮数组对应的任务对象集合，自动执行所有任务
            schedule_tasks_[tick_].clear();
        }

        // 读取定时器文件描述符
        int readTimerFd()
        {
            uint64_t gap = 0;
            ssize_t ret = read(timerfd_, &gap, 8);
            if (ret <= 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                    return 0;
                LOG(Level::Error, "读取定时文件描述符失败");
                exit(static_cast<int>(rs_error::ErrorNum::Timerfd_read_fail));
            }

            return gap;
        }

        // 取消任务
        void cancelTaskInLoop(std::string id)
        {
            // 通过id找到对应的任务
            auto pos = task_map_.find(id);
            if (pos == task_map_.end())
                return;

            per_task_ptr_t pt = pos->second.lock();
            if (pt)
                pt->cancelTask();
        }

        // 新增任务
        void insertTaskInLoop(const std::string &id, uint32_t timeout, const rs_schedule_task::ScheduleTask::main_task_t &task)
        {
            // 构造任务对象
            per_task_ptr_t pt(new rs_schedule_task::ScheduleTask(id, timeout, task));
            pt->setReleaseTask(std::bind(&TimingWheel::removeTask, this, id));
            int pos = (tick_ + timeout) % capacity_;
            schedule_tasks_[pos].push_back(pt);
            task_map_.try_emplace(id, pt);
        }

        // 刷新定时任务
        void refreshTaskInLoop(std::string id)
        {
            // 通过id找到对应的任务
            auto pos = task_map_.find(id);
            if (pos == task_map_.end())
                return;

            // 根据设置任务时给定的超时时间更新任务下一次的超时时间
            per_task_ptr_t pt = pos->second.lock();
            uint32_t timeout = pt->getTimeout();
            int index = (tick_ + timeout) % capacity_;
            schedule_tasks_[index].push_back(pt);
        }

    private:
        int capacity_;                                               // 时间轮数组长度
        int tick_;                                                   // 当前待销毁（执行）的任务
        std::vector<std::vector<per_task_ptr_t>> schedule_tasks_;    // 时间轮数组
        std::unordered_map<std::string, per_task_ptr_t_weak> task_map_; // 管理具体的一个任务，但是不能影响引用计数

        int timerfd_;                                        // 定时器文件描述符
        rs_event_loop_lock_queue::EventLoopLockQueue *loop_; // 监控定时器文件描述符事件
        rs_channel::Channel::ptr timerfd_channel;            // 管理定时器文件描述符事件
    };
}

#endif