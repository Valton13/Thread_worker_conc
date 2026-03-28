# Thread_worker_conc
FlowGraph is a high-performance concurrent task scheduler engineered in Modern C++20, designed to manage complex asynchronous workflows with ease. At its core, the system utilizes a thread pool architecture to execute tasks in parallel, significantly reducing execution time compared to sequential processing. It implements a robust Dependency Graph (DAG) engine, allowing tasks to chain operations safely while automatically detecting circular dependencies to prevent deadlocks. Leveraging advanced C++ features such as type erasure, template metaprogramming, and smart pointers (std::shared_ptr/std::weak_ptr), the scheduler ensures memory safety and supports heterogeneous task return types via std::future and std::promise. With built-in cross-thread exception propagation and atomic state tracking, FlowGraph demonstrates production-grade concurrency patterns suitable for applications ranging from build systems to game engine pipelines.


         WorkFlow of Program:
<img width="393" height="642" alt="wworkflow_threads" src="https://github.com/user-attachments/assets/1ce0d3c0-882b-4d89-b541-d021395e7170" />

    In Depth step by step

  <img width="329" height="618" alt="step_by_step" src="https://github.com/user-attachments/assets/e82ef29c-7dd7-4893-859d-43341c459616" />
       
        
        This project is open source and available for educational purposes.
