#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <unordered_map>

namespace ns_thread_pool
{
    constexpr int kThreadNum = 4;

    template <class T>
    class ThreadPool
    {
    private:
        std::queue<T> task_queue_;
        int thread_num_;

        std::mutex mtx_;
        std::condition_variable cond_;

        std::unordered_map<int, T> busy_tasks_;

        static ThreadPool* ins_;

        ThreadPool(int n = kThreadNum): thread_num_(n)
        {
            InitThread();
        }
    public:
        ThreadPool(ThreadPool& tp) = delete;
        ThreadPool& operator=(ThreadPool& tp) = delete;

        static ThreadPool& GetInstance()
        {
            static std::mutex mtx;
            if (ins_ == nullptr)
            {
                mtx.lock();
                if (ins_ == nullptr)
                {
                    ins_ = new ThreadPool;
                }
                mtx.unlock();
            }

            return *ins_;
        }

        void InitThread()
        {
            for (int i = 0; i < thread_num_; ++i)
            {
                std::thread work_thread = std::thread(ThreadRun);
                work_thread.detach();
            }
        }

        void Push(const T& task)
        {
            std::unique_lock<std::mutex> lock(mtx_);

            if (busy_tasks_.find(task.pev_->fd) == busy_tasks_.end())
            {
                task_queue_.push(task);
                busy_tasks_.insert({task.pev_->fd, T()});

                lock.unlock();
                WakeUp();
            }
            else
            {
                // Log(DEBUG) << "有线程正在处理fd " << task.pev_->fd << endl;y
                busy_tasks_[task.pev_->fd].pev_ = task.pev_;//
                busy_tasks_[task.pev_->fd].ep_events_ |= task.ep_events_;
            }
        }

        bool isEmpty()
        {
            return task_queue_.empty();
        }
    private:
        void WakeUp()
        {
            cond_.notify_one();
        }

        void Wait(std::unique_lock<std::mutex>& lock)
        {
            cond_.wait(lock);
        }

        T Pop()
        {
            std::unique_lock<std::mutex> lock(mtx_);

            while (isEmpty())
            {
                Wait(lock);
            }
            T out = task_queue_.front();
            task_queue_.pop();
            return out;
        }

        void Notify(int key)
        {
            std::unique_lock<std::mutex> lock(mtx_);

            T task = busy_tasks_[key];
            busy_tasks_.erase(key);

            lock.unlock();
            if (task.pev_)
            {
                Push(task);
            }
        }

        static void ThreadRun()
        {
            ThreadPool& tp = ThreadPool::GetInstance();
            while (true)
            {
                T task = tp.Pop();
                int this_fd = task.pev_->fd;
                // Log(DEBUG) << std::this_thread::get_id() << "正在处理fd " << this_fd << endl;
                task();
                // Log(DEBUG) << std::this_thread::get_id() << "处理完成fd " << this_fd << endl;
                ins_->Notify(this_fd);
            }
        }
    };

    template <class T>
    ThreadPool<T>* ThreadPool<T>::ins_ = nullptr;
}
