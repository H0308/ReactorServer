#ifndef __rs_timing_wheel_h__
#define __rs_timing_wheel_h__

#include <vector>
#include <memory>
#include <unordered_map>
#include <reactor_server/schedule_task.h>

namespace timing_wheel
{
    class TimingWheel
    {
    public:
        using per_task_ptr_t = std::shared_ptr<schedule_task::ScheduleTask>;
        using per_task_ptr_t_weak = std::weak_ptr<schedule_task::ScheduleTask>;

        TimingWheel()
            : capacity_(60), tick_(0), schedule_tasks_(capacity_)
        {
        }

        void cancelTask(uint64_t id)
        {
            // 通过id找到对应的任务
            auto pos = task_map_.find(id);
            if (pos == task_map_.end())
                return;

            per_task_ptr_t pt = pos->second.lock();
            if(pt)
                pt->cancelTask();
        }

        void insertTask(uint64_t id, uint32_t timeout, const schedule_task::ScheduleTask::main_task_t &task)
        {
            // 构造任务对象
            per_task_ptr_t pt(new schedule_task::ScheduleTask(id, timeout, task));
            pt->setReleaseTask(std::bind(&TimingWheel::removeTask, this, id));
            int pos = (tick_ + timeout) % capacity_;
            schedule_tasks_[pos].push_back(pt);
            task_map_.try_emplace(id, pt);
        }

        void refreshTask(uint64_t id)
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

        // 执行任务函数，每秒执行1次的函数
        void runTasks()
        {
            tick_ = (tick_ + 1) % capacity_;
            // 销毁时间轮数组对应的任务对象集合，自动执行所有任务
            schedule_tasks_[tick_].clear();
        }

    private:
        // 直接从哈希表中删除对应的任务
        void removeTask(uint64_t id)
        {
            auto pos = task_map_.find(id);
            if (pos == task_map_.end())
                return;

            task_map_.erase(id);
        }

    private:
        int capacity_;                                               // 时间轮数组长度
        int tick_;                                                   // 当前待销毁（执行）的任务
        std::vector<std::vector<per_task_ptr_t>> schedule_tasks_;    // 时间轮数组
        std::unordered_map<uint64_t, per_task_ptr_t_weak> task_map_; // 管理具体的一个任务，但是不能影响引用计数
    };
}

#endif