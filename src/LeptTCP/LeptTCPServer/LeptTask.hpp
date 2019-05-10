/*
v1.0
*/
#ifndef _LEPTTASK_HPP_
#define _LEPTTASK_HPP_

#include<thread>
#include<mutex>
#include<list>

//��������-����
class LeptTask
{
public:
    LeptTask()
    {

    }

    //������
    virtual ~LeptTask()
    {

    }
    //ִ������
    virtual void doTask()
    {

    }
private:

};
typedef std::shared_ptr<LeptTask> LeptTaskPtr;
//ִ������ķ�������
class LeptTaskServer
{
private:
    //��������
    std::list<LeptTaskPtr> _tasks;
    //�������ݻ�����
    std::list<LeptTaskPtr> _tasksBuf;
    //�ı����ݻ�����ʱ��Ҫ����
    std::mutex _mutex;
public:
    //�������
    void addTask(LeptTaskPtr& task)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _tasksBuf.push_back(task);
    }
    //���������߳�
    void Start()
    {
        //�߳�
        std::thread t(std::mem_fn(&LeptTaskServer::OnRun), this);
        t.detach();
    }
protected:
    //��������
    void OnRun()
    {
        while (true)
        {
            //�ӻ�����ȡ������
            if (!_tasksBuf.empty())
            {
                std::lock_guard<std::mutex> lock(_mutex);
                for (auto pTask : _tasksBuf)
                {
                    _tasks.push_back(pTask);
                }
                _tasksBuf.clear();
            }
            //���û������
            if (_tasks.empty())
            {
                std::chrono::milliseconds t(1);
                std::this_thread::sleep_for(t);
                continue;
            }
            //��������
            for (auto pTask : _tasks)
            {
                pTask->doTask();
            }
            //�������
            _tasks.clear();
        }

    }
};
#endif // !_LEPTTASK_HPP_
