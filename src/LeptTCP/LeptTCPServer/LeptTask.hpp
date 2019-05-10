/*
v1.0
*/
#ifndef _LEPTTASK_HPP_
#define _LEPTTASK_HPP_

#include<thread>
#include<mutex>
#include<list>

//任务类型-基类
class LeptTask
{
public:
    LeptTask()
    {

    }

    //虚析构
    virtual ~LeptTask()
    {

    }
    //执行任务
    virtual void doTask()
    {

    }
private:

};
typedef std::shared_ptr<LeptTask> LeptTaskPtr;
//执行任务的服务类型
class LeptTaskServer
{
private:
    //任务数据
    std::list<LeptTaskPtr> _tasks;
    //任务数据缓冲区
    std::list<LeptTaskPtr> _tasksBuf;
    //改变数据缓冲区时需要加锁
    std::mutex _mutex;
public:
    //添加任务
    void addTask(LeptTaskPtr& task)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _tasksBuf.push_back(task);
    }
    //启动工作线程
    void Start()
    {
        //线程
        std::thread t(std::mem_fn(&LeptTaskServer::OnRun), this);
        t.detach();
    }
protected:
    //工作函数
    void OnRun()
    {
        while (true)
        {
            //从缓冲区取出数据
            if (!_tasksBuf.empty())
            {
                std::lock_guard<std::mutex> lock(_mutex);
                for (auto pTask : _tasksBuf)
                {
                    _tasks.push_back(pTask);
                }
                _tasksBuf.clear();
            }
            //如果没有任务
            if (_tasks.empty())
            {
                std::chrono::milliseconds t(1);
                std::this_thread::sleep_for(t);
                continue;
            }
            //处理任务
            for (auto pTask : _tasks)
            {
                pTask->doTask();
            }
            //清空任务
            _tasks.clear();
        }

    }
};
#endif // !_LEPTTASK_HPP_
