#include <iostream>
#include<chrono>
#include<thread>
#include<string>
#include<vector>
#include<atomic>
#include<mutex>
#include<cassert>
#include "ThreadPool.h"

void printTestHeader(const std::string& testName) {
    std::cout<<"\n"<<std::string(60,'=')<<std::endl;
    std::cout << " Test: " << testName << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

void printResult(bool passed , const std::string& message){
    std::cout<<" ["<<(passed ? " pass" : "fail ")<<"]"<<message<<std::endl;
}

void test_simple_dependency(){
    printTestHeader("1. Simple");
    ThreadPool pool(4);
    std::vector<std::string> log;
    std::mutex logMtx;
    auto taskA = std::make_shared<Task<void>>([&]() {
        std::lock_guard<std::mutex> lock(logMtx);
        log.push_back("A executed");
        std::cout << "Task A running on thread " << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });
    taskA->set_ready_callback([&pool](std::shared_ptr<TaskBase> t) { pool.enqueue(t); });
    auto taskB = std::make_shared<Task<void>>([&]() {
        std::lock_guard<std::mutex> lock(logMtx);
        log.push_back("B executed");
        std::cout << " Task B running on thread " << std::this_thread::get_id() << std::endl;
    });
    taskB->set_ready_callback([&pool](std::shared_ptr<TaskBase> t) { pool.enqueue(t); });
    std::static_pointer_cast<TaskBase>(taskB)->add_dependency(std::static_pointer_cast<TaskBase>(taskA));
    std::cout << "  Enqueuing Task A and Task B..." << std::endl;
    pool.enqueue(taskA);
    pool.enqueue(taskB);
    taskB->get_future().get();
    bool aBeforeB = (log.size() == 2 && log[0] == "A executed" && log[1] == "B executed");
    printResult(aBeforeB, "Task A executed before Task B");
    printResult(log.size() == 2, "Both tasks executed exactly once");
}

void test_diamond_dependency(){
    printTestHeader("2. Diamond Dependency");
    ThreadPool pool(4);
    std::vector<std::string> order;
    std::mutex orderMtx;
    auto taskA = std::make_shared<Task<void>>([&]() {
        std::lock_guard<std::mutex> lock(orderMtx);
        order.push_back("A");
        std::cout << " Task A Executed" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    });
    taskA->set_ready_callback([&pool](std::shared_ptr<TaskBase> t) { pool.enqueue(t); });
    auto taskB = std::make_shared<Task<void>>([&]() {
        std::lock_guard<std::mutex> lock(orderMtx);
        order.push_back("B");
        std::cout << " Task B Executed" << std::endl;
    });
    taskB->set_ready_callback([&pool](std::shared_ptr<TaskBase> t) { pool.enqueue(t); });
    auto taskC = std::make_shared<Task<void>>([&]() {
        std::lock_guard<std::mutex> lock(orderMtx);
        order.push_back("C");
        std::cout << " Task C Executed" << std::endl;
    });
    taskC->set_ready_callback([&pool](std::shared_ptr<TaskBase> t) { pool.enqueue(t); });
    auto taskD = std::make_shared<Task<void>>([&]() {
        std::lock_guard<std::mutex> lock(orderMtx);
        order.push_back("D");
        std::cout << " Task D Executed (depends on B and C)" << std::endl;
    });
    taskD->set_ready_callback([&pool](std::shared_ptr<TaskBase> t) { pool.enqueue(t); });
    std::static_pointer_cast<TaskBase>(taskB)->add_dependency(std::static_pointer_cast<TaskBase>(taskA));
    std::static_pointer_cast<TaskBase>(taskC)->add_dependency(std::static_pointer_cast<TaskBase>(taskA));
    std::static_pointer_cast<TaskBase>(taskD)->add_dependency(std::static_pointer_cast<TaskBase>(taskB));
    std::static_pointer_cast<TaskBase>(taskD)->add_dependency(std::static_pointer_cast<TaskBase>(taskC));
    std::cout << "  Enqueuing all 4 tasks..." << std::endl;
    pool.enqueue(taskA);
    pool.enqueue(taskB);
    pool.enqueue(taskC);
    pool.enqueue(taskD);
    taskD->get_future().get();
    bool aFirst = (order.front() == "A");
    bool dLast = (order.back() == "D");
    bool allFour = (order.size() == 4);
    std::cout << "  Execution order: ";
    for (const auto& s : order) std::cout << s << " ";
    std::cout << std::endl;
    printResult(aFirst, "Task A executed first");
    printResult(dLast, "Task D executed last");
    printResult(allFour, "All 4 tasks executed");
}

void test_cycle_detection(){
    printTestHeader("3.Cycle detection");
    auto callback =[](std::shared_ptr<TaskBase> t){};
    std::cout << "\n   Testing NO CYCLE case " << std::endl;
    try{
        auto taskX = std::make_shared<Task<void>>([](){});
        taskX->set_ready_callback(callback);
        auto taskY = std::make_shared<Task<void>>([]() {});
        taskY->set_ready_callback(callback);
        auto taskZ = std::make_shared<Task<void>>([]() {});
        taskZ->set_ready_callback(callback);
        printResult(true, "No cycle detected (correct - this is valid)");
    } catch (const std::exception& e) {
        printResult(false, "Unexpected exception: " + std::string(e.what()));
    }

    std::cout << "\n  Testing DIRECT CYCLE case" << std::endl;
    bool directCycleCaught = false;
    try{
        auto taskA = std::make_shared<Task<void>>([]() {});
        taskA->set_ready_callback(callback);
        auto taskB = std::make_shared<Task<void>>([]() {});
        taskB->set_ready_callback(callback);
        taskB->add_dependency(taskA); 
        taskA->add_dependency(taskB);
    }  catch (const std::runtime_error& e) {
        directCycleCaught = true;
        std::cout << "  Caught: " << e.what() << std::endl;
    }
    printResult(directCycleCaught, "Direct cycle detected and rejected");
}

void test_return_values() {
    printTestHeader("4.Return values");
    ThreadPool pool(4);
    std::cout<<"\n test for int"<<std::endl;
    auto futureInt = pool.submit<int>([]() {
        std::cout << " int task " << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return 42;
    });
    int intResult = futureInt.get();
    std::cout << "  int task Result: " << intResult << std::endl;
    printResult(intResult == 42, "int return value correct (42)");
    std::cout << "\n   Testing string" << std::endl;
    auto futureStr = pool.submit<std::string>([]() {
        std::cout << "    [string task] Building string..." << std::endl;
        return std::string("Hello from my Side");
    });
    std::string strResult = futureStr.get();  
    std::cout << "   string task Result: \"" << strResult << "\"" << std::endl;
    printResult(strResult == "Hello from my Side", "string return value correct");
}

void test_exception_propogation(){
    printTestHeader("5.Exception Propogation");
    ThreadPool pool(4);
    auto parent = std::make_shared<Task<void>>([]() {
        std::cout << "  Parent Running " << std::endl;
        throw std::runtime_error("Parent task failed!");
    });
    parent->set_ready_callback([&pool](std::shared_ptr<TaskBase> t) { pool.enqueue(t); }); 
    auto child = std::make_shared<Task<void>>([]() {
        std::cout << "  Child " << std::endl;
    });
    child->set_ready_callback([&pool](std::shared_ptr<TaskBase> t) { pool.enqueue(t); });
    std::static_pointer_cast<TaskBase>(child)->add_dependency(std::static_pointer_cast<TaskBase>(parent));
    std::cout << "  Connenct parent and child..." << std::endl;
    pool.enqueue(parent);
    pool.enqueue(child);
    bool childFailed = false;
    try {
        child->get_future().get();
    } catch (...){
        childFailed = true;
        std::cout << "  Child Caught exception " << std::endl;
    }
    printResult(childFailed, "Child task failed due to parent failure");
    printResult(child->is_failed(), "Child task state is FAILED");
    printResult(parent->is_failed(), "Parent task state is FAILED");
}

void test_task_states() {
    printTestHeader("6. Task state tracking");
    ThreadPool pool(4);
    std::cout<<"\n test sucess task state"<<std::endl;
    auto successTask = std::make_shared<Task<int>>([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 100;
    });
    successTask->set_ready_callback([&pool](std::shared_ptr<TaskBase> t) { pool.enqueue(t); });
    std::cout << "    Initial state: " << (successTask->get_state() == TaskState::Pending ? "Pending" : "Other") << std::endl;
    printResult(successTask->get_state() == TaskState::Pending, "Initial state is Pending");
    pool.enqueue(successTask);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout << " Final state: " << (successTask->get_state() == TaskState::Completed ? "Completed" : "Other") << std::endl;
    printResult(successTask->get_state() == TaskState::Completed, "Final state is Completed");
    printResult(successTask->get_future().get() == 100, "Result value is correct");
}

void test_stress_verbose() {
    printTestHeader("7. Stress test for 50 tasks");
    ThreadPool pool(8);
    std::atomic<int> done(0);
    std::vector<std::string> output_log;
    std::mutex logMtx;    
    std::cout << "Submitting 50 tasks..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 50; ++i) {
        pool.submit<void>([i, &done, &output_log, &logMtx]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10 + (i % 3) * 10));
            {
                std::lock_guard<std::mutex> lock(logMtx);
                output_log.push_back("[Task " + std::to_string(i) + "] Done");
            }
            done++;
        });
    }
    while (done.load() < 50) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "\nTask completion log (first 10):" << std::endl;
    for (size_t j = 0; j < std::min(output_log.size(), size_t(10)); ++j) {
        std::cout << "  " << output_log[j] << '\n';
    }
    if (output_log.size() > 10) {
        std::cout << "  ... and " << (output_log.size() - 10) << " more" << std::endl;
    }
    std::cout << "\nStress test summary:" << std::endl;
    std::cout << "  Tasks completed: " << done.load() << "/50" << std::endl;
    std::cout << "  Time taken: " << ms << "ms" << std::endl;
    std::cout << "  Avg time per task: " << (ms / 50.0) << "ms" << std::endl;
    printResult(done.load() == 50, "All tasks completed");
    printResult(ms < 500, "Completed in reasonable time (<500ms indicates parallelism)");
}

void test_basic_execution(){
    printTestHeader("8. Basic");
    ThreadPool pool(4);
    std::atomic<bool> executed{false};
    std::cout<<"submit a task"<<std::endl;
    auto future = pool.submit<void>([&executed]() {
        std::cout << "  Basic task running!" << std::endl;
        executed = true;
    });
    future.get();
    printResult(executed.load() , "Task done");
    printResult(true, "No crashes or hangs");
}

void test_parallel_execution(){
    printTestHeader("9.parallel example");
    ThreadPool pool(4);
    std::atomic<int> counter{0};
    std::vector<std::chrono::steady_clock::time_point> timestamps;
    std::mutex tsMtx;
    std::cout << " submitting 20 tasks that each take 50ms" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 20; ++i) {
        pool.submit<void>([i, &counter, &timestamps, &tsMtx]() {
            auto taskStart = std::chrono::steady_clock::now();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            counter++;
            //auto taskEnd = std::chrono::steady_clock::now();
            {
                std::lock_guard<std::mutex> lock(tsMtx);
                timestamps.push_back(taskStart);
            }
        });
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));
    auto end = std::chrono::high_resolution_clock::now();
    auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "\n  Parallelism analysis:" << std::endl;
    std::cout << " Tasks completed: " << counter.load() << "/20" << std::endl;
    std::cout << " total time: " << totalMs << "ms" << std::endl;
    printResult(counter.load() == 20, "all 20 tasks completed");
    printResult(totalMs < 500, "Execution was parallel ");
    if (timestamps.size() >= 4) {
        std::cout << "\n  Sample task start times :" << std::endl;
        auto first = timestamps[0];
        for (int i = 0; i < std::min(4, (int)timestamps.size()); ++i) {
            auto offset = std::chrono::duration_cast<std::chrono::milliseconds>(timestamps[i] - first).count();
            std::cout << " Task " << i << " started at +" << offset << "ms" << std::endl;
        }
    }
}

int main(){
    std::cout << "\n" << std::string(60, '#') << std::endl;
    std::cout << " ThreadPool Comprehensive Test Suite" << std::endl;
    std::cout << std::string(60, '#') << std::endl;
    test_basic_execution();      
    test_return_values();           
    test_simple_dependency();      
    test_diamond_dependency();       
    test_cycle_detection();           
    test_exception_propogation();     
    test_task_states();               
    test_parallel_execution();        
    test_stress_verbose();   
    std::cout << "\n" << std::string(60, '#') << std::endl;
    std::cout << " ALL TESTS COMPLETE" << std::endl;
    std::cout << std::string(60, '#') << std::endl;
    return 0;

}