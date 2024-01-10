#include "ParallelTasks.hpp"
#include <iostream>
class MyRunner : public ITask
{
public:
    static int val;
    void runTask(TaskID task_id, int i, int n)
    {
        printf("val = %d (i = %d, n = %d)\n", ++val, i, n);
    }
    ~MyRunner() {}
};

class OtherRunner : public ITask
{
public:
    ~OtherRunner() {}
    void runTask(TaskID task_id, int i, int n)
    {
        printf("Other runner: %d\n", i);
    }
};
int MyRunner::val = 0;
int main()
{
    ParallelTasks parallel(4);
    MyRunner runner;
    OtherRunner OtherRunner;
    TaskID A = parallel.addTaskWithDeps(&runner, 5, std::vector<TaskID>{});
    TaskID B = parallel.addTaskWithDeps(&runner, 4, std::vector<TaskID>{A});
    parallel.addTaskWithDeps(&OtherRunner, 1, {A, B});
    parallel.addTaskWithDeps(&OtherRunner, 2, std::vector<TaskID>{});
    parallel.sync();  // run all tasks
}