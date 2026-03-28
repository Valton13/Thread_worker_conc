#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include<vector>
#include<queue>
#include<thread>
#include<mutex>
#include<condition_variable>
#include<functional>
#include<future>
#include <memory>
#include "TaskBase.h"
#include "Task.h"

class ThreadPool{
    public:
        explicit ThreadPool(size_t num_threads);
        ~ThreadPool();
        template<typename T , typename F>
        std::future<T> submit(F&& func);
        void enqueue(std::shared_ptr<TaskBase> task);
    private:
        void worker_thread();
        std::vector<std::thread> workers;
        std::queue<std::shared_ptr<TaskBase>>tasks;
        std::mutex queue_mutex;
        std::condition_variable condition;
        std::atomic <bool> stop;
};
template<typename T ,typename F>
std::future<T> ThreadPool::submit(F&& func){
    auto task = std::make_shared<Task<T>>(std::forward<F>(func));
    std::function<void(std::shared_ptr<TaskBase>)> callback = 
        [this](std::shared_ptr<TaskBase> t) {
            this->enqueue(t);
        };
    task->set_ready_callback(callback);
    std::future<T> fut = task->get_future();
    enqueue(task);
    return fut;
}



#endif