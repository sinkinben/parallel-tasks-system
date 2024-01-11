#include "ParallelTasks.hpp"
#include <iostream>
class MyRunner : public ITask
{
public:
    void runTask(TaskID task_id, int i, int n)
    {
        printf("TaskID = %u (idx = %d, total jobs = %d) MuRunner \n", task_id, i, n);
    }
    ~MyRunner() {}
};

class OtherRunner : public ITask
{
public:
    ~OtherRunner() {}
    void runTask(TaskID task_id, int i, int n)
    {
        printf("TaskID = %u (idx = %d, total jobs = %d) OtherRunner \n", task_id, i, n);
    }
};

int main()
{
    ParallelTasks parallel(16);
    MyRunner runner;
    OtherRunner OtherRunner;
    TaskID A = parallel.addTaskWithDeps(&runner, 5, std::vector<TaskID>{});  // Task 'A' in graph, which has 5 jobs
    TaskID B = parallel.addTaskWithDeps(&runner, 4, std::vector<TaskID>{A}); // Task 'B' has 4 jobs
    parallel.addTaskWithDeps(&OtherRunner, 1, {A, B});                       // Task 'C' has 1 job
    parallel.addTaskWithDeps(&OtherRunner, 2, std::vector<TaskID>{});        // Isolated task 'D' (a isolated vertex in graph), has 2 jobs
    parallel.sync();                                                         // run all tasks
}

/* This program build such a task graph:
        A -> B -> C
        |         ^
        +---------+
        
        D
 */