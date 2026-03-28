#ifndef TASK_H
#define TASK_H

#include "TaskBase.h"
#include<future>
#include<functional>
#include<iostream>

template<typename T>
class Task:public TaskBase{
    public :
        explicit Task(std:: function<T()> work);
        std::future<T> get_future();
        void execute() override;
    private:
        std::function<T()> work_function;
        std::promise<T> promise;
        bool future_retrived = false;
};
template<typename T>
Task<T>::Task(std::function<T()> work):work_function(std::move(work)){
}
template<typename T>
std::future<T> Task<T>::get_future(){
    if(future_retrived){
        throw std::logic_error("love u ");
    }
    future_retrived = true;
    return promise.get_future();
}

template<typename T>
void Task<T>::execute(){
    state.store(TaskState::Running);
    try{
        if (work_function){
            promise.set_value(work_function());
        }
        state.store(TaskState::Completed);
    }catch(...){
        exception = std::current_exception();
        promise.set_exception(exception);
        state.store(TaskState::Failed);
    }
    notify_children();
}

template<>
class Task<void> : public TaskBase{
    public:
        explicit Task(std::function<void()> work);
        std::future<void> get_future();
        void execute() override;
    private:
        std::function<void()> work_function;
        std::promise<void> promise;
        bool future_retireved = false;
};

inline Task<void>::Task(std::function<void()> work): work_function(std::move(work)){
}
inline std::future<void> Task<void>::get_future(){
    if (future_retireved){
        throw std::logic_error("future already retrieved");
    }
    future_retireved   = true;
    return promise.get_future();
}

inline void Task<void>::execute() {
    state.store(TaskState::Running);
    try{
        if (work_function){
            work_function();
            
        } 
        promise.set_value();
        state.store(TaskState::Completed);
    }catch (...) {
        exception = std::current_exception();
        promise.set_exception(std::current_exception());
        state.store(TaskState::Failed);
    }
    notify_children();
}


#endif
