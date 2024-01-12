## Parallel DAG Tasks System

A parallel tasks computing system (with DAG dependency), implemented via sleeping-thread-pool.

<br/>

**Usage**

Add `itask.h` and `ParallelTasks.hpp` into project, and include `ParallelTasks.hpp`.

<br/>

**Getting Start**

It's very easy to use `ParallelTasks` class to parallelize multi-tasks.

First, define a customed task class `MyTask` (must inherit the interface `ITask`), and implement the `runTask` method.

```cpp
class MyTask : public ITask
{
public:
    void runTask(TaskID task_id, int job_idx, int num_jobs)
    {
        printf("TaskID = %u (idx = %d, total jobs = %d) MyRunner \n", task_id, job_idx, num_jobs);
    }
};

class AnotherTask : public ITask
{
    // ...
};
```

The `TaskID` is assigned by class `ParallelTasks` (for debug purpose), in most cases, we do not need to care about it. 

And we can write `main` program like this:

```cpp
int main()
{
    ParallelTasks parallel(4);  // 4 threads to execute the tasks
    MyTask *task = new MyTask();
    AnotherTask *another = new AnotherTask();

    TaskID A = parallel.addTaskWithDeps(task, 1, {});      // task has only 1 job
    TaskID B = parallel.addTaskWithDeps(another, 1, {A});  // B must be executed after A

    parallel.sync();  // execute the tasks we added above
}
```

<br/>

**Simple Example**

See `example-simple.cpp`, and we can type `make simple` to build it.

In this program, we construct a task graph like this:

<img src="https://raw.githubusercontent.com/Sin-Kinben/PicGo/master/img/simple-graph.png" style="width: 50%;"/>

And task `A, B, C, D` have `{5, 4, 1, 2}` sub-jobs, respectively. This example shows how does `ParallelTasks` system works.


<br/>

**Merge Sort Example**

See `example-merge-sort.cpp`, and we can type `make sorting` to build it.

This program implements a multi-threads merge-sorting algorithm by `ParallelTasks` system.


