#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t num_threads):stop(false){
    for (size_t i =0 ; i< num_threads; i++){
        workers.emplace_back([this] { this->worker_thread() ;});
    }
}

ThreadPool:: ~ThreadPool(){
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread& worker : workers){
        worker.join();
    }
}
void ThreadPool::enqueue(std::shared_ptr<TaskBase> task) {
    if (task->is_ready()) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            tasks.push(task);
        }
        condition.notify_one();
    }
}

void ThreadPool::worker_thread(){
    while(true){
        std::shared_ptr<TaskBase>task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            condition.wait(lock , [this]{
                return stop || !tasks.empty();
            });
            if (stop && tasks.empty()){
                return ;
            }
            task = tasks.front();
             tasks.pop();
        }
        task->execute();
    }
}

