#include <iostream>
#include <unistd.h>
#include <reactor_server/timing_wheel.h>

using namespace std;
using namespace timing_wheel;
using namespace schedule_task;

class Task
{
public:
    Task()
    {
        std::cout << "构造执行" << std::endl;
    }

    ~Task()
    {
        std::cout << "析构执行" << std::endl;
    }
};

// 任务函数
void deleteTask(Task *t)
{
    delete t;
}

int main()
{
    Task *t = new Task;
    TimingWheel tw;
    // 插入一个5秒后执行任务函数的任务
    tw.insertTask(1, 5, std::bind(&deleteTask, t));

    // 每秒刷新1次任务，刷新5次
    for(int i = 0; i < 5; i++)
    {
        tw.refreshTask(1);
        tw.runTasks();
        sleep(1);
        std::cout << "刷新一次任务" << std::endl;
    }

    // 取消任务
    tw.cancelTask(1);

    while (true)
    {
        // 执行任务
        sleep(1);
        std::cout << "-------------" << std::endl;
        tw.runTasks();
    }

    return 0;
}