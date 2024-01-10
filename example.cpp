#include "ParallelTasks.hpp"
#include <iostream>
class MyRunner : public ITask
{
public:
    static int val;
    void runTask(int i, int n)
    {
        printf("val = %d (i = %d, n = %d)\n", ++val, i, n);
    }
    ~MyRunner() {}
};

class OtherRunner : public ITask
{
public:
    ~OtherRunner() {}
    void runTask(int i, int n)
    {
        printf("Other runner: %d\n", i);
    }
};
int MyRunner::val = 0;
int main()
{
    int n = 3;
    ParallelTasks parallel(4);
    MyRunner runner;
    OtherRunner OtherRunner;
    int A = parallel.runTasksAsyncWithDeps(&runner, 5, std::vector<int>{});
    int B = parallel.runTasksAsyncWithDeps(&runner, 4, std::vector<int>{A});
    parallel.runTasksAsyncWithDeps(&OtherRunner, 1, {A, B});
    // parallel.runTasksAsyncWithDeps(&runner, n, std::vector<int>{B});
    parallel.sync();
}