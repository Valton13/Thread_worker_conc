#include "TaskBase.h"
#include<iostream>

static std::atomic<int> global_task_id{0};

bool TaskBase::can_reach(std::shared_ptr<TaskBase> current , std::shared_ptr<TaskBase>target , std::set<int>& visited){
    if (current == target){
        return true;
    }
    visited.insert(current->task_id);
    std::lock_guard<std::mutex> lock(current->dependencies_mutex);
    for ( auto& dep_weak : current->dependencies){
        if (auto dep = dep_weak.lock()){
            if(visited.find(dep->task_id) == visited.end()){
                if(can_reach(dep , target , visited)){
                    return true;
                }
            }
        }
    }
    return false;
}

void TaskBase::add_dependency(std::shared_ptr<TaskBase> parent){
    std::set<int> visited;
    if (can_reach(shared_from_this() , parent , visited)){
        throw std::runtime_error("Cycle detected: Task " + 
                                 std::to_string(parent->task_id) + " depends on Task " + 
                                 std::to_string(task_id));
    }
    dependency_count++;
    {
        std::lock_guard<std::mutex>lock(dependencies_mutex);
        dependencies.push_back(parent);
    }
    {
        std::lock_guard<std::mutex> lock(parent->children_mutex);
        parent->children.push_back(shared_from_this());
    }
}

bool TaskBase::is_ready() const{
    return dependency_count == 0;
}

void TaskBase::set_ready_callback(std::function<void(std::shared_ptr<TaskBase>)>callback){
    on_task_ready = std::move(callback);
}
void TaskBase::set_failed(){
    state.store(TaskState::Failed);
}
void TaskBase::notify_children(){
    std::vector<std::shared_ptr<TaskBase>>ready_children;
    {
        std::lock_guard<std::mutex> lock(children_mutex);
        for (auto it = children.begin() ; it != children.end() ;){
            if (auto child = it->lock()){
                int new_count = --child->dependency_count;
                if (state.load() == TaskState::Failed){
                    child->exception = exception;
                    child->set_failed();
                }
                if (new_count == 0){
                    ready_children.push_back(child);
                }
                ++it;
            }else{
                it = children.erase(it);
            }
        }
    }
    if (on_task_ready){
        for (auto& child : ready_children){
            on_task_ready(child);
        }
    }
}