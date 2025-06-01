#ifndef __rs_schedule_task_h__
#define __rs_schedule_task_h__

#include <cstdint>
#include <functional>

namespace schedule_task
{
    // 定时任务类型
    class ScheduleTask
    {
    public:
        using main_task_t = std::function<void()>; // 主任务，具体任务类型未知，如果有任务，上层需要通过绑定设置参数
        using release_task_t = std::function<void()>; // 释放任务

        ScheduleTask(uint64_t id, uint32_t timeout, const main_task_t &m_task)
            : id_(id), timeout_(timeout), m_task_(m_task), isCanceled(false)
        {

        }

        /**
         * 取消任务不能直接从任务管理集合中移除，否则会因为执行销毁而执行任务
         * 此时就变成了提前执行任务而不是销毁任务
         * 需要从任务本身出发，先确保任务被取消时不会执行任务函数
         */
        void cancelTask()
        {
            isCanceled = true;
        }

        void setReleaseTask(const release_task_t& r_task)
        {
            r_task_ = r_task;
        }

        // 获取到延迟时间
        uint32_t getTimeout()
        {
            return timeout_;
        }

        ~ScheduleTask()
        {
            // 任务被取消时不执行
            if(!isCanceled)
                m_task_();
            r_task_();
        }

    private:
        uint64_t id_; // 任务编号，统一分配，当前类中不决定id值
        uint32_t timeout_; // 超时时间
        main_task_t m_task_; // 主任务类型
        release_task_t r_task_; // 释放任务
        bool isCanceled; // 任务是否已经被取消
    };
}

#endif