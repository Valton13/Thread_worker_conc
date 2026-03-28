#ifndef TASK_BASE_H 
#define TASK_BASE_H

#include <memory>
#include<vector>
#include<atomic>
#include<mutex>
#include<functional>
#include<set>
#include<stdexcept>
#include<exception>

enum class TaskState{
    Pending,
    Running,
    Completed,
    Failed
};

class TaskBase;
class TaskBase : public std::enable_shared_from_this<TaskBase>{
    public:
        virtual ~TaskBase() = default;
        virtual void execute() = 0;
        void add_dependency(std::shared_ptr<TaskBase>parent);
        bool is_ready() const;
        int get_id() const { return task_id ; }
        void set_ready_callback(std::function<void(std::shared_ptr<TaskBase>)> callback);
        TaskState get_state() const {return state.load() ;}
        void set_failed() ;
        bool is_failed() const { return state.load() == TaskState::Failed;}
    protected:
        void notify_children();
        TaskBase() = default;
        bool can_reach(std::shared_ptr<TaskBase> current ,std::shared_ptr<TaskBase> target , std::set<int>& visited);
        int task_id;
        std::atomic<int> dependency_count{0};
        std::atomic<TaskState>state;
        std::exception_ptr exception;
        std::vector<std::weak_ptr<TaskBase>> children;
        std::vector<std::weak_ptr<TaskBase>>dependencies;
        std::mutex children_mutex;
        std::mutex dependencies_mutex;
        std::function<void(std::shared_ptr<TaskBase>)> on_task_ready;

};
#endif