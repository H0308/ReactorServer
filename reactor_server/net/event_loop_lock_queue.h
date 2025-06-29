#ifndef __rs_event_loop_lock_queue_h__
#define __rs_event_loop_lock_queue_h__

#include <sys/eventfd.h>
#include <reactor_server/base/log.h>
#include <reactor_server/net/channel.h>
#include <reactor_server/net/poller.h>
#include <reactor_server/base/error.h>
#include <reactor_server/net/timing_wheel.h>

namespace rs_event_loop_lock_queue
{
    using namespace rs_log_system;

    // 任务类型
    using task_t = std::function<void()>;
    
    class EventLoopLockQueue
    {
    public:
        using ptr = std::shared_ptr<EventLoopLockQueue>;

        EventLoopLockQueue()
            :thread_id_(std::this_thread::get_id()),
            event_fd_(getEventId()),
            event_fd_channel_(std::make_shared<rs_channel::Channel>(this, event_fd_)),
            poller_(std::make_shared<rs_poller::Poller>()),
            timing_wheel_(std::make_shared<rs_timing_wheel::TimingWheel>(this))
        {
            // 为事件通知描述符绑定回调函数，并启用可读事件监控
            event_fd_channel_->setReadCallback(std::bind(&EventLoopLockQueue::readEventId, this));
            event_fd_channel_->enableConcerningReadFd();
        }

        // 启动事件监控
        void startEventLoop()
        {
            while (true)
            {
                std::vector<rs_channel::Channel::ptr> channels;
                // 1. 启动事件监控
                int nfds = poller_->startEpoll(channels);
                // 2. 进行事件处理
                std::for_each(channels.begin(), channels.end(), [](const rs_channel::Channel::ptr &channel){
                    channel->handleEvent();
                });

                executeAllTasksInQueue();
            }
        }

        // 执行任务，如果在当前线程，就直接执行任务，否则将任务插入到任务队列
        void runTasks(const task_t &task)
        {
            if(isInCurrentThread())
            {
                // 当前线程中直接执行任务
                task(); 
                return;
            }

            // 否则插入到任务队列
            enqueue(task);
        }
        
        void enqueue(const task_t &task)
        {
            // 任务入队列
            {
                std::unique_lock<std::mutex> lock(tasks_mutex_);
                tasks_.emplace_back(task);
            }

            // 防止执行流阻塞在epoll_wait，使用时间事件通知的方式触发可读事件跳出epoll_wait
            writeEventId();
        }

        // ? 为什么不需要将任务弹出任务队列
        // ! 因为根据下面的设计，是将当前任务队列中的对象转移到一个临时局部任务队列中
        // ! 一旦该函数执行完毕，对应的临时局部任务队列也就被销毁，从而致使已经执行的任务被销毁
        // void dequeue(const task_t &task)
        // {
        // }

        void updateEvent(rs_channel::Channel::ptr channel)
        {
            poller_->updateEvent(channel);
        }

        void removeEvent(rs_channel::Channel::ptr channel)
        {
            poller_->removeEvent(channel);
        }

        void cancelTask(const std::string &id)
        {
            timing_wheel_->cancelTask(id);
        }

        void insertTask(const std::string &id, uint32_t timeout, const rs_schedule_task::ScheduleTask::main_task_t &task)
        {
            timing_wheel_->insertTask(id, timeout, task);
        }

        void refreshTask(const std::string &id)
        {
            timing_wheel_->refreshTask(id);
        }

        // 非线程安全，使用时需要保证在同一线程内
        bool hasTimer(const std::string &id)
        {
            return timing_wheel_->hasTimer(id);
        }

        // 断言判断是否在当前线程内
        void assertInCurrentThread()
        {
            assert(std::this_thread::get_id() == thread_id_);
        }

    private:
        // 判断当前是否在EventLoop所在线程
        bool isInCurrentThread()
        {
            return (std::this_thread::get_id() == thread_id_);
        }
    
        // 创建并获取事件通知文件描述符
        static int getEventId()
        {
            // 必须是8字节
            uint64_t init_val = 1;
            int efd = eventfd(init_val, EFD_CLOEXEC | EFD_NONBLOCK);
            if(efd < 0)
            {
                LOG(Level::Error, "事件通知文件描述符创建失败");
                exit(static_cast<int>(rs_error::ErrorNum::EventFd_create_fail));
            }

            return efd;
        }

        // 读取事件通知文件描述符
        void readEventId()
        {
            uint64_t val = 0;
            ssize_t ret = read(event_fd_, &val, sizeof(uint64_t));
            if(ret <= 0)
            {
                if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                    return;
                LOG(Level::Error, "读取事件通知文件描述符失败");
                exit(static_cast<int>(rs_error::ErrorNum::EventFd_read_fail));
            }
        }

        void writeEventId()
        {
            uint64_t val = 1;
            ssize_t ret = write(event_fd_, &val, sizeof(uint64_t));
            if (ret <= 0)
            {
                if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                    return;
                LOG(Level::Error, "写入事件通知文件描述符失败");
                exit(static_cast<int>(rs_error::ErrorNum::EventFd_write_fail));
            }
        }

        // 执行任务队列中所有的任务
        void executeAllTasksInQueue()
        {   
            std::vector<task_t> tasks;
            {
                std::unique_lock<std::mutex> lock(tasks_mutex_);
                tasks_.swap(tasks);
            }

            std::for_each(tasks.begin(), tasks.end(), [](const task_t &task){
                task();
            });
        }

    private:
        std::thread::id thread_id_; // 当前EventLoop所在线程的线程id
        int event_fd_; // 事件通知描述符
        rs_channel::Channel::ptr event_fd_channel_; // 事件通知描述符事件监控结构
        rs_poller::Poller::ptr poller_; // 事件监控模块
        std::vector<task_t> tasks_; // 任务队列
        std::mutex tasks_mutex_; // 保护任务队列互斥锁

        rs_timing_wheel::TimingWheel::ptr timing_wheel_; // 时间轮
    };
}

// 分离实现Channel类中的函数
void rs_channel::Channel::update()
{
    loop_->updateEvent(shared_from_this());
}

void rs_channel::Channel::remove()
{
    loop_->removeEvent(shared_from_this());
}

// 分离实现TimingWheel中的函数
void rs_timing_wheel::TimingWheel::cancelTask(const std::string &id)
{
    loop_->runTasks(std::bind(&TimingWheel::cancelTaskInLoop, this, id));
}

void rs_timing_wheel::TimingWheel::insertTask(const std::string &id, uint32_t timeout, const rs_schedule_task::ScheduleTask::main_task_t &task)
{
    loop_->runTasks(std::bind(&TimingWheel::insertTaskInLoop, this, id, timeout, task));
}

void rs_timing_wheel::TimingWheel::refreshTask(const std::string &id)
{
    loop_->runTasks(std::bind(&TimingWheel::refreshTaskInLoop, this, id));
}

#endif